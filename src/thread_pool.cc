//
// Created by Jiamin Huang on 5/16/16.
//

#include "thread_pool.h"

#include <iostream>

using std::cout;
using std::endl;

thread_pool::thread_pool(DB &db) : thread_pool(db, 5) {
}

thread_pool::thread_pool(DB &db, size_t pool_size) {
    for (size_t count = 0; count < pool_size; ++count) {
        workers.emplace_back(db);
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

void thread_pool::submit_task(task *db_task) {
    auto min_job_worker = &(workers.front());
    for (auto &worker : workers) {
        if (worker.num_jobs() < min_job_worker->num_jobs()) {
            min_job_worker = &worker;
        }
    }
    min_job_worker->submit_job(db_task);
}
