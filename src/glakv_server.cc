#include "DBImpl.h"
#include "config.h"
#include "exponential_distribution.h"
#include "thread_pool.h"
#include "fakeDB.h"

#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>

#include <atomic>
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <unordered_map>

#define BUF_LEN     (VAL_LEN * 2)
#define GET         "Get"
#define PUT         "Put"
#define DEL         "Del"
#define QUIT        "Quit"
#define NUM_CLIENTS 128
#define NUM_EXP     100000
#define INT_LEN     (sizeof(uint64_t) / sizeof(char))

using std::atomic;
using std::cerr;
using std::cout;
using std::endl;
using std::min;
using std::ostream;
using std::rand;
using std::string;
using std::mutex;
using std::thread;
using std::chrono::time_point;
using std::uniform_int_distribution;
using std::vector;
using std::unordered_map;

static bool quit = false;
static bool prefetch = false;
static int num_prefetch = NUM_PREFETCH;
static vector<thread> clients;
static atomic<int> num_clients(0);
static bool reported = true;
static double prediction_hit = 0;
static double prefetch_hit = 0;
static double queue_size = 0;

void quit_server(int) {
    cout << endl << "Receives CTRL-C, quiting..." << endl;
    quit = true;
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void usage(ostream &os) {
    os << "Usage: glakv [OPTIONS]" << endl;
    os << "[OPTIONS]:" << endl;
    os << "--help" << endl;
    os << "-h       show this message" << endl;
    os << "--dir" << endl;
    os << "-d       directory to store the database files" << endl;
}

void parse_opts(int argc, char *argv[], int &help_flag, string &dir, int &num_exp, double &cache_size, int &num_threads) {
    struct option long_options[] = {
            {"help",    no_argument,       0, 'h'},
            {"prefetch",optional_argument, 0, 'p'},
            {"num",     required_argument, 0, 'n'},
            {"dir",     required_argument, 0, 'd'},
            {"cache",   required_argument, 0, 'c'},
            {"threads", required_argument, 0, 't'},
            {0, 0, 0, 0}
    };

    int c;
    int option_index;
    help_flag = 0;
    dir = "glakv_home";
    while ((c = getopt_long(argc, argv, "hn:p:d:c:t:", long_options, &option_index)) != -1) {
        switch(c) {
            case 'h':
                help_flag = 1;
                break;
            case 'p':
                prefetch = true;
                if (optarg) {
                    num_prefetch = atoi(optarg);
                }
                break;
            case 'n':
                num_exp = atoi(optarg);
                break;
            case 'd':
                dir = optarg;
                break;
            case 'c':
                cache_size = atof(optarg) / 100;
                break;
            case 't':
                num_threads = atoi(optarg);
                break;
            case '?':
                break;
            default:
                usage(cerr);
                break;
        }
    }
}

int setup_server() {
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }
    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 4242;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }
    return sockfd;
}

void store_uint32(char *buf, uint32_t val) {
    *((uint32_t *) buf) = val;
}

uint32_t get_uint32(char *buf) {
    return *((uint32_t *) buf);
}

void store_uint64(char *buf, uint64_t val) {
    *((uint64_t *) buf) = val;
}

uint64_t get_uint64(char *buf) {
    return *((uint64_t *) buf);
}

void prefetch_for_key(DB &db, thread_pool &pool, uint32_t key, list<task> &prefetch_tasks) {
    if (prefetch) {
        for (int count = 0; count < num_prefetch; ++count) {
            uint32_t prediction = (key + count + db.size() / 3) % db.size();
            prefetch_tasks.push_back({fetch, prediction, [] (bool, string &, double) {}});
            task &db_task = prefetch_tasks.back();
            pool.submit_task_ref(db_task);
        }
    }
}

bool prefetch_or_submit(int sockfd, thread_pool &pool, DB &db, vector<double> &latencies, mutex &lock,
                        uint32_t key, list<task> &prefetch_tasks, string &val) {
    cout << "Queue size is " << prefetch_tasks.size() << endl;
    auto callback = [&key, &db, &sockfd, &prefetch_tasks, &latencies, &lock, &pool] (bool success, string &value, double time) {
        char res[BUF_LEN];
        uint64_t res_len = 0;
        if (success) {
            res[0] = 1;
            store_uint64(res + 1, value.size());
            memcpy(res + 1 + INT_LEN, value.c_str(), value.size());
            res_len = 1 + INT_LEN + value.size();
            prefetch_for_key(db, pool, key, prefetch_tasks);
        } else {
            res[0] = 0;
            res_len = 1;
        }
        if (write(sockfd, res, res_len) != (ssize_t) res_len) {
            cerr << "Error sending result to client" << endl;
            return;
        }
        lock.lock();
        latencies.push_back(time);
        lock.unlock();
        cout << "Response sent back to client" << endl;
    };
    queue_size += prefetch_tasks.size();
    bool prefetch_success = false;
    bool prediction_success = false;
    auto iter = prefetch_tasks.begin();
    while (iter != prefetch_tasks.end()) {
        if (iter->key == key) {
            prediction_success = true;
            prediction_hit++;
            if (iter->task_state == finished) {
                prefetch_success = true;
                val = iter->val;
                prefetch_hit++;
                iter = prefetch_tasks.erase(iter);
            } else {
                iter->callback = [&prefetch_tasks, &iter, &callback] (bool success, string &value, double time) {
                    callback(success, value, time);
                    prefetch_tasks.erase(iter);
                };
                iter->birth_time = std::chrono::high_resolution_clock::now();
                ++iter;
                cout << "Promote prefetch as get" << endl;
            }
        } else if (iter->task_state == in_queue) {
            iter->operation = noop;
            iter->callback = [&prefetch_tasks, &iter] (bool, string &, double) {
                prefetch_tasks.erase(iter);
            };
            ++iter;
        } else if (iter->task_state == processing) {
            iter->callback = [&prefetch_tasks, &iter] (bool, string &, double) {
                prefetch_tasks.erase(iter);
            };
            ++iter;
        } else {
            iter = prefetch_tasks.erase(iter);
        }
    }
    if (!prediction_success) {
        cout << "Prediction is not successful; submit task." << endl;
        pool.submit_task({get, key, std::move(callback)});
    }
    cout << "Prefetch is " << (prefetch_success ? "" : "not") << " successful" << endl;
    return prefetch_success;
}

void serve_client(int sockfd, thread_pool &pool, DB &db, vector<double> &latencies, mutex &lock) {
    list<task> prefetch_tasks;
    reported = false;
    char buffer[BUF_LEN];
    uint32_t key;
    while (!quit) {
        bzero(buffer, BUF_LEN);
        if (read(sockfd, buffer, BUF_LEN) < 0) {
            cerr << "Error reading from client" << endl;
            break;
        }
        size_t GET_LEN = strlen(GET);
        size_t PUT_LEN = strlen(PUT);
        size_t DEL_LEN = strlen(DEL);
        size_t QUIT_LEN = strlen(QUIT);
        if (strncmp(GET, buffer, GET_LEN) == 0) {
            key = get_uint32(buffer + GET_LEN);
            string val;
            auto start = std::chrono::high_resolution_clock::now();
            if (prefetch_or_submit(sockfd, pool, db, latencies, lock, key, prefetch_tasks, val)) {
                auto diff = std::chrono::high_resolution_clock::now() - start;
                double time = diff.count();
                prefetch_for_key(db, pool, key, prefetch_tasks);
                char res[BUF_LEN];
                uint64_t res_len = 0;
                res[0] = 1;
                store_uint64(res + 1, val.size());
                memcpy(res + 1 + INT_LEN, val.c_str(), val.size());
                res_len = 1 + INT_LEN + val.size();
                write(sockfd, res, res_len);
                lock.lock();
                latencies.push_back(time);
                lock.unlock();
            }
        } else if (strncmp(PUT, buffer, PUT_LEN) == 0) {
            key = get_uint32(buffer + PUT_LEN);
            uint64_t vlen = get_uint64(buffer + PUT_LEN + KEY_LEN);
            char *val_buf = buffer + PUT_LEN + KEY_LEN + INT_LEN;
            string value(val_buf, vlen);
            pool.submit_task({put, key, value, [&sockfd, &latencies, &lock] (bool success, string &val, double time) {
                if (write(sockfd, &success, 1) != 1) {
                    cerr << "Error sending result to client" << endl;
                    return;
                }
                lock.lock();
                latencies.push_back(time);
                lock.unlock();
            }});
        } else if (strncmp(DEL, buffer, DEL_LEN) == 0) {
            key = get_uint32(buffer + DEL_LEN);
            pool.submit_task({del, key, [&sockfd, &latencies, &lock] (bool success, string &val, double time) {
                if (write(sockfd, &success, 1) != 1) {
                    cerr << "Error sending result to client" << endl;
                    return;
                }
                lock.lock();
                latencies.push_back(time);
                lock.unlock();
            }});
        } else if (strncmp(QUIT, buffer, QUIT_LEN) == 0) {
            break;
        }
    }
    --num_clients;
    close(sockfd);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, quit_server);

    int help_flag = 0;
    string dir = "glakv_home";
    int num_exp = 0;
    double cache_size = CACHE_SIZE;
    int num_workers = 10;
    parse_opts(argc, argv, help_flag, dir, num_exp, cache_size, num_workers);

    if (help_flag) {
        usage(cout);
        return 0;
    }

    fakeDB db(dir, num_prefetch);
    int sockfd = setup_server();
    int newsockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr;
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    vector<double> latencies;
    thread_pool pool(db, (size_t) num_workers);
    mutex lock;
    int count = 0;
    while (!quit) {
        newsockfd = accept(sockfd,
                           (struct sockaddr *) &cli_addr,
                           &clilen);
        if (newsockfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (num_clients == 0 && !reported) {
                    double sum = 0;
                    for (auto latency : latencies) {
                        sum += latency;
                    }
                    cout << sum / latencies.size() << "," << latencies.size() << endl;
//                    cout << "Prediction hits: " << prediction_hit << endl;
//                    cout << "Prefetch hits: " << prefetch_hit << endl;
//                    cout << "Average queue size: " << queue_size / latencies.size() << endl;
                    latencies.clear();
                    prediction_hit = 0;
                    prefetch_hit = 0;
                    reported = true;
                    if (num_exp > 0) {
                        ++count;
                        if (count == num_exp) {
                            break;
                        }
                    }
                }
                usleep(100);
                continue;
            } else {
                error("ERROR on accept");
            }
        }
        flags = fcntl(newsockfd, F_GETFL, 0);
        fcntl(newsockfd, F_SETFL, flags & ~O_NONBLOCK);
        thread t(serve_client, newsockfd, std::ref(pool), std::ref(db), std::ref(latencies), std::ref(lock));
        clients.push_back(std::move(t));
        ++num_clients;
    }

    for (auto &t : clients) {
        t.join();
    }
    close(sockfd);
    return 0;
}
