//
// Created by Jiamin Huang on 5/14/16.
//

#include "DBImpl.h"

#include <sys/stat.h>
#include <fcntl.h>

#define ENTRY_SIZE ((uint64_t) 1 + VAL_LEN)
#define BUF_SIZE   ((DB_SIZE * ENTRY_SIZE) / 2000)
#define MIN(x, y)  ((x) < (y) ? (x) : (y))

using std::ifstream;
using std::ofstream;

DBImpl::DBImpl(string &dir) : DBImpl(dir, CACHE_SIZE) {}

DBImpl::DBImpl(string &dir, uint64_t cache_size) : cache(cache_size) {
    if (dir[dir.size() - 1] != '/') {
        dir += '/';
    }
    create_if_not_exists(dir);
    dir += "glakv.db";
    db_file = open(dir.c_str(), O_RDWR);
    read(db_file, &(DBImpl::db_size), sizeof(uint32_t));
}

DBImpl::~DBImpl() {
    close(db_file);
}

bool DBImpl::get(uint32_t key, string &val) {
    if (!cache.get(key, val)) {
        unique_lock<shared_mutex> lock(mutex);
        uint64_t offset = sizeof(uint32_t) + key * ENTRY_SIZE;
        lseek(db_file, offset, SEEK_SET);
        char buffer[ENTRY_SIZE];
        read(db_file, buffer, ENTRY_SIZE);
        lock.unlock();
        if (buffer[0] == 1) {
            val = string(buffer + 1, VAL_LEN);
            cache.put(key, val);
            return true;
        }
        return false;
    }
    return true;
}

void DBImpl::put(uint32_t key, string &val) {
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
    write(db_file, &(DBImpl::db_size), sizeof(uint32_t));
    lseek(db_file, offset, SEEK_SET);
    write(db_file, buffer, ENTRY_SIZE);
    fsync(db_file);
}

void DBImpl::del(uint32_t key) {
    cache.del(key);
    unique_lock<shared_mutex> lock(mutex);
    uint64_t offset = sizeof(uint32_t) + key * ENTRY_SIZE;
    lseek(db_file, offset, SEEK_SET);
    char in_use = 0;
    write(db_file, &in_use, 1);
    fsync(db_file);
}

void DBImpl::create_if_not_exists(string &dir) {
    ifstream file(dir + "glakv.db");
    if (file.fail()) {
        // Need to create a new file
        ofstream db_file(dir + "glakv.db");
        uint64_t total_size = sizeof(uint32_t) + UINT32_MAX * ENTRY_SIZE;
        const uint64_t count = 2000;
        uint64_t buf_size = total_size / count;
        char *buf = new char[buf_size];
        bzero(buf, buf_size);
        while (total_size > 0) {
            uint64_t write_size = MIN(total_size, buf_size);
            db_file.write(buf, (std::streamsize) write_size);
            total_size -= write_size;
        }
        db_file.close();
    }
}

uint32_t DBImpl::size() {
    return db_size;
}
