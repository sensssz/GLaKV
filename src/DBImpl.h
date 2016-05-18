//
// Created by Jiamin Huang on 5/14/16.
//

#ifndef GLAKV_DB_IMPL_H
#define GLAKV_DB_IMPL_H

#include "config.h"
#include "DB.h"

#include <fstream>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>


#define CACHE_SIZE (DB_SIZE / 50)

using std::fstream;
using std::list;
using std::pair;
using std::string;
using std::unordered_map;

using boost::shared_mutex;
using boost::shared_lock;
using boost::unique_lock;

class LRUCache {
private:
    typedef pair<string, list<uint32_t>::iterator> val_t;
    unordered_map<uint32_t, val_t> cache;
    list<uint32_t> lru_keys;
    shared_mutex mutex;
    uint64_t capacity;
    uint32_t size;
    LRUCache(const LRUCache &) = delete;
public:
    LRUCache(uint64_t capacity_in) : capacity(capacity_in), size(0) {}

    bool put(uint32_t key, string &val) {
        unique_lock<shared_mutex> lock(mutex);
        auto map_iter = cache.find(key);
        if (map_iter != cache.end()) {
            // Already in the cache
            map_iter->second.first = val;
            lru_keys.erase(map_iter->second.second);
            lru_keys.push_front(key);
            map_iter->second.second = lru_keys.begin();
            return true;
        } else {
            if (size == capacity) {
                auto last_key = lru_keys.back();
                lru_keys.pop_back();
                cache.erase(last_key);
                --size;
            }
            ++size;
            lru_keys.push_front(key);
            cache[key] = {val, lru_keys.begin()};
            return false;
        }
    }

    bool get(uint32_t key, string &val) {
        shared_lock<shared_mutex> read_lock(mutex);
        auto map_iter = cache.find(key);
        if (map_iter != cache.end()) {
            val = cache[key].first;
            lru_keys.erase(map_iter->second.second);
            lru_keys.push_front(key);
            map_iter->second.second = lru_keys.begin();
            return true;
        }
        return false;
    }

    void del(uint32_t key) {
        unique_lock<shared_mutex> lock(mutex);
        auto map_iter = cache.find(key);
        if (map_iter != cache.end()) {
            lru_keys.erase(map_iter->second.second);
            cache.erase(key);
            --size;
        }
    }
};

class DBImpl : public DB {
private:
    shared_mutex mutex;
    int          db_file;
    uint32_t     db_size;
    LRUCache     cache;

    DBImpl(const DBImpl&) = delete;
    void create_if_not_exists(string &dir);
public:
    DBImpl(string &dir);
    DBImpl(string &dir, uint64_t cache_size);
    ~DBImpl();
    virtual bool get(uint32_t key, string &val);
    virtual void put(uint32_t key, string &val);
    virtual void del(uint32_t key);
    uint32_t size();
};


#endif //GLAKV_DB_IMPL_H