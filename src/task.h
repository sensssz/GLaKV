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

enum opcode {get, put, del, noop};

struct task {
    opcode      operation;
    uint32_t    key;
    char       *val;
    size_t      vlen;
    std::chrono::high_resolution_clock::time_point  birth_time;
    function<void(bool, string&, double)> callback;

    task() : operation(noop), key(0), val(nullptr), vlen(0) {
        birth_time = std::chrono::high_resolution_clock::now();
    }
    task(opcode op, uint32_t key_in, function<void(bool, string &, double)> &&callback_in)
            : operation(op), key(key_in), val(nullptr),
              vlen(0), callback(std::move(callback_in)) {
        birth_time = std::chrono::high_resolution_clock::now();
    }
    task(opcode op, uint32_t key_in, char *val_in, size_t vlen_in, function<void(bool, string &, double)> &&callback_in)
            : operation(op), key(key_in), val(val_in),
              vlen(vlen_in), callback(std::move(callback_in)) {
        birth_time = std::chrono::high_resolution_clock::now();
    }
};

#endif //GLAKV_TASK_H
