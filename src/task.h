//
// Created by Jiamin Huang on 5/16/16.
//

#ifndef GLAKV_TASK_H
#define GLAKV_TASK_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

using std::function;
using std::string;

enum opcode {get, put, del, fetch, noop};
enum state {in_queue, processing, finished};

struct task {
    opcode      operation;
    uint32_t    key;
    string      val;
    state       task_state;
    std::chrono::high_resolution_clock::time_point  birth_time;
    function<void(bool, string&, double)> callback;

    task() : operation(noop), key(0) {
        task_state = in_queue;
        birth_time = std::chrono::high_resolution_clock::now();
    }
    task(opcode op, uint32_t key_in, function<void(bool, string &, double)> &&callback_in)
            : operation(op), key(key_in), callback(std::move(callback_in)) {
        task_state = in_queue;
        birth_time = std::chrono::high_resolution_clock::now();
    }
    task(opcode op, uint32_t key_in, string val_in, function<void(bool, string &, double)> &&callback_in)
            : operation(op), key(key_in), val(val_in), callback(std::move(callback_in)) {
        task_state = in_queue;
        birth_time = std::chrono::high_resolution_clock::now();
    }
    task(const task &rhs)
            : operation(rhs.operation), key(rhs.key), val(rhs.val),
              task_state(rhs.task_state), birth_time(rhs.birth_time), callback(rhs.callback) {}
};

#endif //GLAKV_TASK_H
