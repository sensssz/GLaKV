//
// Created by Jiamin Huang on 5/16/16.
//

#ifndef GLAKV_WORKER_THREAD_H
#define GLAKV_WORKER_THREAD_H

#include "DB.h"
#include "task.h"
#include "concurrentqueue.h"

#include <thread>

using std::thread;
using moodycamel::ConcurrentQueue;

class worker_thread {
private:
    thread worker;
    bool quit;
public:
    worker_thread(ConcurrentQueue<task> &queue, DB &db);
    void stop();
};


#endif //GLAKV_WORKER_THREAD_H
