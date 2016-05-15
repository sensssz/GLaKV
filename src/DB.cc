//
// Created by Jiamin Huang on 5/14/16.
//

#include "DB.h"

#include <sys/stat.h>
#include <fcntl.h>

#define BUF_SIZE   536870912
#define ENTRY_SIZE (1 + VAL_LEN)

using std::ifstream;
using std::ofstream;

DB::DB(string &dir) : DB(dir, CACHE_SIZE) {}

DB::DB(string &dir, uint64_t cache_size) : cache(cache_size) {
    if (dir[dir.size() - 1] != '/') {
        dir += '/';
    }
    create_if_not_exists(dir);
    dir += "glakv.db";
    db_file = open(dir.c_str(), O_RDWR);
    read(db_file, &(DB::db_size), sizeof(uint32_t));
}

DB::~DB() {
    close(db_file);
}

bool DB::get(uint32_t key, string &val) {
    if (!cache.get(key, val)) {
        shared_lock<shared_mutex> read_lock(mutex);
        uint64_t offset = sizeof(uint32_t) + key * ENTRY_SIZE;
        lseek(db_file, offset, SEEK_SET);
        char buffer[1 + VAL_LEN];
        read(db_file, buffer, VAL_LEN);
        read_lock.unlock();
        if (buffer[0] == 1) {
            val = string(buffer + 1 + KEY_LEN, VAL_LEN);
            cache.put(key, val);
            return true;
        }
        return false;
    }
    return true;
}

void DB::put(uint32_t key, string &val) {
    cache.put(key, val);

    char in_use = 0;
    char buffer[ENTRY_SIZE];
    buffer[0] = 1;      // Entry is in use
    memcpy(buffer + 1, val.data(), val.size());

    unique_lock<shared_mutex> lock(mutex);
    uint64_t offset = sizeof(uint32_t) + key * ENTRY_SIZE;

    // Update size if necessary
    lseek(db_file, offset, SEEK_SET);
    read(db_file, &in_use, 1);
    if (!in_use) {
        ++db_size;
    }

    // Update kv-pair and size
    lseek(db_file, 0, SEEK_SET);
    write(db_file, &(DB::db_size), sizeof(uint32_t));
    lseek(db_file, offset, SEEK_SET);
    write(db_file, buffer, ENTRY_SIZE);
    fsync(db_file);
}

void DB::del(uint32_t key) {
    cache.del(key);
    unique_lock<shared_mutex> lock(mutex);
    uint64_t offset = sizeof(uint32_t) + key * ENTRY_SIZE;
    lseek(db_file, offset, SEEK_SET);
    char in_use = 0;
    write(db_file, &in_use, 1);
    fsync(db_file);
}

void DB::create_if_not_exists(string &dir) {
    ifstream file(dir + "glakv.db");
    if (file.fail()) {
        // Need to create a new file
        ofstream db_file(dir + "glakv.db");
        char *buf = new char[BUF_SIZE];
        bzero(buf, BUF_SIZE);
        uint64_t total_size = sizeof(uint32_t) + UINT32_MAX * ENTRY_SIZE;
        uint64_t count = total_size / BUF_SIZE;
        for (uint64_t i = 0; i < count; ++i) {
            db_file.write(buf, BUF_SIZE);
        }
        db_file.close();
    }
}

uint32_t DB::size() {
    return db_size;
}















