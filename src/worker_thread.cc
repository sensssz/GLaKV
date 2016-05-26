//
// Created by Jiamin Huang on 5/16/16.
//

#include "worker_thread.h"

#include <string>
#include <chrono>
#include <iostream>
#include <mutex>

using std::cout;
using std::endl;
using std::string;
using std::unique_lock;
using std::chrono::microseconds;

worker_thread::worker_thread(ConcurrentQueue<task *> &queue, DB &db_in)
        : task_queue(queue), quit(false), db(db_in) {}

worker_thread::worker_thread(worker_thread &&other)
        : task_queue(other.task_queue), worker(std::move(other.worker)),
          quit(other.quit), db(other.db) {}

void worker_thread::start() {
    worker = thread([this] {
        task *db_task = nullptr;
        while (!quit) {
            if (!task_queue.try_dequeue(db_task)) {
                std::this_thread::sleep_for(microseconds(10));
                continue;
            }
            unique_lock<mutex> lock(db_task->task_mutex);
            db_task->task_state = processing;
            bool success = true;
            switch (db_task->operation) {
                case get:
                    success = db.get(db_task->key, db_task->val);
                    break;
                case put:
                    db.put(db_task->key, db_task->val);
                    break;
                case del:
                    db.del(db_task->key);
                    break;
                case fetch:
                    success = db.get(db_task->key, db_task->val);
                    break;
                case noop:
                    break;
                default:
                    break;
            }
            auto end = std::chrono::high_resolution_clock::now();
            auto diff = end - db_task->birth_time;
            db_task->callback(success, db_task->val, diff.count() / 1000);
            db_task->task_state = finished;
            lock.unlock();
            db_task->task_state = detached;
        }
    });
}

void worker_thread::set_stop() {
    quit = true;
}

void worker_thread::join() {
    worker.join();
}
