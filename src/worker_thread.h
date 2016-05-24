//
// Created by Jiamin Huang on 5/16/16.
//

#ifndef GLAKV_WORKER_THREAD_H
#define GLAKV_WORKER_THREAD_H

#include "DB.h"
#include "task.h"
#include "concurrentqueue.h"

#include <mutex>
#include <thread>
#include <condition_variable>

using std::mutex;
using std::thread;
using std::condition_variable;
using moodycamel::ConcurrentQueue;

class worker_thread {
private:
    ConcurrentQueue<task *> &task_queue;
    thread worker;
    bool quit;
    DB &db;
public:
    worker_thread(ConcurrentQueue<task *> &queue, DB &db);
    worker_thread(worker_thread &&other);
    void start();
    void set_stop();
    void join();
};


#endif //GLAKV_WORKER_THREAD_H
