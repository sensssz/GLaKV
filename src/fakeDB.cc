//
// Created by Jiamin Huang on 5/17/16.
//

#include "config.h"
#include "fakeDB.h"

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <chrono>
#include <thread>
#include <iostream>

using std::cout;
using std::endl;
using std::chrono::microseconds;

const int CONTENTION = 50;
const int GET_TIME = 300;
const int PUT_TIME = 150;
const int DEL_TIME = 100;

fakeDB::fakeDB(string dir, uint32_t num_prefetch_in) : num_prefetch(num_prefetch_in) {
    if (dir[dir.size() - 1] != '/') {
        dir += '/';
    }
    dir += "glakv.db";
    int db_file = open(dir.c_str(), O_RDONLY);
    assert(db_file > 0);
    read(db_file, &(fakeDB::db_size), sizeof(uint32_t));
    close(db_file);
}

bool fakeDB::get(uint32_t key, string &val) {
    std::this_thread::sleep_for(microseconds(GET_TIME + num_prefetch * CONTENTION));
    char value[VAL_LEN];
    memset(value, 0x42, VAL_LEN);
    val = string(value, VAL_LEN);
    return true;
}

void fakeDB::put(uint32_t key, string &val) {
    cache.put(key, val);
    unique_lock<shared_mutex> lock(mutex);
    std::this_thread::sleep_for(microseconds(PUT_TIME));
}

void fakeDB::del(uint32_t key) {
    cache.del(key);
    unique_lock<shared_mutex> lock(mutex);
    std::this_thread::sleep_for(microseconds(DEL_TIME));
}

uint32_t fakeDB::size() {
    return db_size;
}
