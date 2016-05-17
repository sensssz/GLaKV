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
using std::chrono::microseconds;

worker_thread::worker_thread(ConcurrentQueue<task> &queue, bool &quit_in, DB &db)
        : quit(quit_in), worker([&queue, &db, this] {
            task db_task;
            while (!quit) {
                if (!queue.try_dequeue(db_task)) {
                    std::this_thread::sleep_for(microseconds(2));
                    continue;
                }
                string val;
                bool success = true;
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
                    case fetch:
                        break;
                    case noop:
                        break;
                    default:
                        break;
                }
                if (db_task.operation != fetch ||
                    db_task.operation != noop) {
                    auto end = std::chrono::high_resolution_clock::now();
                    auto diff = end - db_task.birth_time;
                    db_task.callback(success, val, diff.count());
                }
            }
        }) {}

worker_thread::worker_thread(worker_thread &&other)
        : quit(other.quit), worker(std::move(other.worker)) {}

void worker_thread::join() {
    worker.join();
}
