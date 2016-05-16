//
// Created by Jiamin Huang on 5/16/16.
//

#include "worker_thread.h"

#include <string>
#include <chrono>
#include <iostream>

using std::cout;
using std::endl;
using std::string;
using std::unique_lock;
using std::chrono::microseconds;

worker_thread::worker_thread(ConcurrentQueue<task> &queue, DB &db, mutex &queue_mutex, condition_variable &cv)
        : quit(false), worker([&queue, &db, &queue_mutex, &cv, this] () {
            task db_task;
            while (!quit) {
                if (!queue.try_dequeue(db_task)) {
                    unique_lock lock(queue_mutex);
                    cv.wait(lock, [&queue, &db_task] {queue.try_dequeue(db_task);});
                    continue;
                }
                cout << "Task retrieved. Processing..." << endl;
                string val;
                bool success = true;
                auto start = std::chrono::high_resolution_clock::now();
                switch (db_task.operation) {
                    case get:
                        success = db.get(db_task.key, val);
                        break;
                    case put:
                        val = string(db_task.val, db_task.vlen);
                        db.put(db_task.key, val);
                        break;
                    case del:
                        db.del(db_task.key);
                        break;
                    case noop:
                        break;
                    default:
                        break;
                }
                auto end = std::chrono::high_resolution_clock::now();
                auto diff = end - start;
                cout << "Invoking callback" << endl;
                db_task.callback(success, val, diff.count());
                cout << "Task finished" << endl;
            }
        }) {}

worker_thread::worker_thread(worker_thread &&other)
        : quit(other.quit), worker(std::move(other.worker)) {}

void worker_thread::stop() {
    quit = true;
    worker.join();
}



