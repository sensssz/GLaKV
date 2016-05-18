//
// Created by Jiamin Huang on 5/17/16.
//

#include "fakeDB.h"

#include <chrono>
#include <thread>

using std::chrono::microseconds;

const int GET_TIME = 300;
const int PUT_TIME = 900;
const int DEL_TIME = 600;

bool fakeDB::get(uint32_t key, string &val) {
    std::this_thread::sleep_for(GET_TIME);
    return false;
}

void fakeDB::put(uint32_t key, string &val) {
    std::this_thread::sleep_for(PUT_TIME);
}

void fakeDB::del(uint32_t key) {
    std::this_thread::sleep_for(DEL_TIME);
}

uint32_t fakeDB::size() {
    return 0;
}
