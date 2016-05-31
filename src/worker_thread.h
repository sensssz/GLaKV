//
// Created by Jiamin Huang on 5/16/16.
//

#ifndef GLAKV_WORKER_THREAD_H
#define GLAKV_WORKER_THREAD_H

#include "DB.h"
#include "task.h"
#include "mpsc_queue.h"

#include <mutex>
#include <thread>
#include <condition_variable>

using std::mutex;
using std::thread;
using std::condition_variable;

class worker_thread {
private:
    mpsc_queue<task *> task_queue;
    uint32_t size;
    thread worker;
    bool quit;
    DB &db;

    worker_thread(worker_thread &&other) : db(other.db) {}
public:
    worker_thread(DB &db);
    void start();
    void set_stop();
    void join();
    void submit_job(task *db_task);
    uint32_t num_jobs();
};


#endif //GLAKV_WORKER_THREAD_H
