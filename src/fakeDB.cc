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

using std::chrono::microseconds;

const int GET_TIME = 20000;
const int PUT_TIME = 40000;
const int DEL_TIME = 30000;

fakeDB::fakeDB(string dir) {
    if (dir[dir.size() - 1] != '/') {
        dir += '/';
    }
    int db_file = open(dir.c_str(), O_RDONLY);
    assert(db_file > 0);
    read(db_file, &(fakeDB::db_size), sizeof(uint32_t));
    close(db_file);
}

bool fakeDB::get(uint32_t key, string &val) {
    if (!cache.get(key, val)) {
        unique_lock<shared_mutex> lock(mutex);
        std::this_thread::sleep_for(microseconds(GET_TIME));
        lock.unlock();
        char buffer[VAL_LEN];
        val = string(buffer + 1, VAL_LEN);
        cache.put(key, val);
    }
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
