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
#include <future>
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
static vector<thread> threads;
static atomic<int> num_threads(0);
static bool reported = true;
static double read_perc = 1;

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

void parse_opts(int argc, char *argv[], int &help_flag, string &dir, int &num_exp, double &cache_size) {
    struct option long_options[] = {
            {"help",    no_argument,       0, 'h'},
            {"prefetch",optional_argument, 0, 'p'},
            {"num",     required_argument, 0, 'n'},
            {"dir",     required_argument, 0, 'd'},
            {"cache",   required_argument, 0, 'c'},
            {"read",    required_argument, 0, 'r'},
            {0, 0, 0, 0}
    };

    int c;
    int option_index;
    help_flag = 0;
    dir = "glakv_home";
    while ((c = getopt_long(argc, argv, "hn:p:d:c:r:", long_options, &option_index)) != -1) {
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
            case 'r':
                read_perc = atof(optarg) / 100;
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

void prefetch_for_key(DB &db, thread_pool &pool, uint32_t key, unordered_map<uint32_t, string> &prefetch_cache) {
    if (prefetch) {
        uint32_t original_key = key;
        for (int count = 0; count < num_prefetch; ++count) {
            key = (original_key + count + db.size() / 3) % db.size();
            pool.submit_task({fetch, key, [&prefetch_cache, &count] (bool success, string &val, double) {
                prefetch_cache[count] = val;
            }});
        }
    }
}

bool check_prefetch_cache(uint32_t key, unordered_map<uint32_t, string> &prefetch_cache, string &val) {
    auto iter = prefetch_cache.find(key);
    if (iter == prefetch_cache.end()) {
        return false;
    }
    val = iter->second;
    prefetch_cache.clear();
    return true;
}

void serve_client(int sockfd, thread_pool &pool, DB &db, vector<double> &latencies, mutex &lock) {
    unordered_map<uint32_t, string> prefetch_cache((unsigned long) (2 * num_prefetch));
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
            if (check_prefetch_cache(key, prefetch_cache, val)) {
                write(sockfd, val.data(), val.length());
                lock.lock();
                latencies.push_back(0);
                lock.unlock();
                prefetch_for_key(db, pool, key, prefetch_cache);
                continue;
            }
            pool.submit_task({get, key, [&key, &db, &sockfd, &latencies, &lock, &pool, &prefetch_cache] (bool success, string &val, double time) {
                char res[BUF_LEN];
                uint64_t res_len = 0;
                if (success) {
                    res[0] = 1;
                    store_uint64(res + 1, val.size());
                    memcpy(res + 1 + INT_LEN, val.c_str(), val.size());
                    res_len = 1 + INT_LEN + val.size();
                    prefetch_for_key(db, pool, key, prefetch_cache);
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
            }});
        } else if (strncmp(PUT, buffer, PUT_LEN) == 0) {
            key = get_uint32(buffer + PUT_LEN);
            uint64_t vlen = get_uint64(buffer + PUT_LEN + KEY_LEN);
            char *val_buf = buffer + PUT_LEN + KEY_LEN + INT_LEN;
            pool.submit_task({put, key, val_buf, vlen, [&sockfd, &latencies, &lock] (bool success, string &val, double time) {
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
    --num_threads;
    close(sockfd);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, quit_server);

    int help_flag = 0;
    string dir;
    int num_exp = 0;
    double cache_size = CACHE_SIZE / 100;
    parse_opts(argc, argv, help_flag, dir, num_exp, cache_size);

    if (help_flag) {
        usage(cout);
        return 0;
    }

    fakeDB db(dir);
    int sockfd = setup_server();
    int newsockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr;
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    vector<double> latencies;
    thread_pool pool(db, (uint32_t) (cache_size * db.size()));
    mutex lock;
    int count = 0;
    while (!quit) {
        newsockfd = accept(sockfd,
                           (struct sockaddr *) &cli_addr,
                           &clilen);
        if (newsockfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (num_threads == 0 && !reported) {
                    double sum = 0;
                    for (auto latency : latencies) {
                        sum += latency;
                    }
                    cout << sum / latencies.size() << endl;
                    latencies.clear();
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
        threads.push_back(std::move(t));
        ++num_threads;
    }

    for (auto &t : threads) {
        t.join();
    }
    close(sockfd);
    return 0;
}