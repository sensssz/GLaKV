#include "DB.h"
#include "config.h"
#include "exponential_distribution.h"
#include "thread_pool.h"

#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>

#include <atomic>
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <future>
#include <fcntl.h>

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

static bool quit = false;
static bool prefetch = false;
static int num_prefetch = NUM_PREFETCH;
static vector<thread> threads;
static atomic<int> num_threads(0);
static bool reported = true;

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

void parse_opts(int argc, char *argv[], int &help_flag, string &dir, int &num_exp) {
    struct option long_options[] = {
            {"help",    no_argument,       0, 'h'},
            {"prefetch",optional_argument, 0, 'p'},
            {"num",     required_argument, 0, 'n'},
            {"dir",     required_argument, 0, 'd'},
            {0, 0, 0, 0}
    };

    int c;
    int option_index;
    help_flag = 0;
    dir = "glakv_home";
    while ((c = getopt_long(argc, argv, "hn:p:d:", long_options, &option_index)) != -1) {
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

void prefetch_kv(DB &db, uint32_t key) {
    string val;
    db.get(key, val);
}

void prefetch_for_key(DB &db, uint32_t key) {
    if (prefetch) {
        uint32_t original_key = key;
        for (int count = 0; count < num_prefetch; ++count) {
            key = (original_key + count + db.size() / 3) % db.size();
            thread t(prefetch_kv, std::ref(db), key);
            t.detach();
        }
    }
}

void serve_client(int sockfd, thread_pool &pool, DB &db, vector<double> &latencies, mutex &lock) {
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
            pool.submit_task({get, key, [&key, &db, &sockfd, &latencies, &lock] (bool success, string &val, double time) {
                char res[BUF_LEN];
                uint64_t res_len = 0;
                if (success) {
                    res[0] = 1;
                    store_uint64(res + 1, val.size());
                    memcpy(res + 1 + INT_LEN, val.c_str(), val.size());
                    res_len = 1 + INT_LEN + val.size();
                    prefetch_for_key(db, key);
                } else {
                    res[0] = 0;
                    res_len = 1;
                }
                if (write(sockfd, res, res_len) != res_len) {
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
    parse_opts(argc, argv, help_flag, dir, num_exp);

    if (help_flag) {
        usage(cout);
        return 0;
    }

    DB db(dir);
    int sockfd = setup_server();
    int newsockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr;
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    vector<double> latencies;
    thread_pool pool(db);
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