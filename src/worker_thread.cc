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

worker_thread::worker_thread(ConcurrentQueue<task> &queue, DB &db_in, int num_prefetch_in)
        : task_queue(queue), num_prefetch(num_prefetch_in), quit(false), db(db_in) {}

worker_thread::worker_thread(worker_thread &&other, int num_prefetch_in)
        : task_queue(other.task_queue), num_prefetch(num_prefetch_in),
          worker(std::move(other.worker)), quit(other.quit), db(other.db) {}

void worker_thread::start() {
    worker = thread([this] {
        task db_task;
        while (!quit) {
            if (!task_queue.try_dequeue(db_task)) {
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
                    cout << db_task.key << ": ";
                    for (int count = 0; count < num_prefetch; ++count) {
                        uint32_t prediction = (db_task.key + count + db.size()) % db.size();
                        cout << prediction << ',';
                        success = db.get(prediction, val);
                        db_task.callback(success, val, prediction);
                    }
                    cout << endl;
                    break;
                case noop:
                    break;
                default:
                    break;
            }
            if (db_task.operation != fetch &&
                db_task.operation != noop) {
                auto end = std::chrono::high_resolution_clock::now();
                auto diff = std::chrono::duration_cast<microseconds>(end - db_task.birth_time);
                db_task.callback(success, val, diff.count());
            }
        }
    });
}

void worker_thread::set_stop() {
    quit = true;
}

void worker_thread::join() {
    worker.join();
}
