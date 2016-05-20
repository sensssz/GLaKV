//
// Created by Jiamin Huang on 5/16/16.
//

#include "thread_pool.h"

#include <iostream>

using std::cout;
using std::endl;

thread_pool::thread_pool(DB &db, int num_prefetch) : thread_pool(db, 5, num_prefetch) {
}

thread_pool::thread_pool(DB &db, size_t pool_size, int num_prefetch) {
    for (size_t count = 0; count < pool_size; ++count) {
        workers.emplace_back(task_queue, db, num_prefetch);
    }
    for (auto &worker : workers) {
        worker.start();
    }
}

thread_pool::~thread_pool() {
    for (auto &worker : workers) {
        worker.set_stop();
    }
    for (auto &worker : workers) {
        worker.join();
    }
}

void thread_pool::submit_task(task db_task) {
    task_queue.enqueue(std::move(db_task));
}
