//
// Created by Jiamin Huang on 5/16/16.
//

#include "thread_pool.h"

thread_pool::thread_pool(DB &db) : thread_pool(db, 5) {
}

thread_pool::thread_pool(DB &db, size_t pool_size) {
    for (size_t count = 0; count < pool_size; ++count) {
        workers.push_back(worker_thread(task_queue, db));
    }
}

thread_pool::~thread_pool() {
    for (auto &worker : workers) {
        worker.stop();
    }
}

void thread_pool::submit_task(task &&db_task) {
    task_queue.enqueue(db_task);
}
