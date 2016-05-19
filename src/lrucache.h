//
// Created by Jiamin Huang on 5/19/16.
//

#ifndef GLAKV_LRUCACHE_H
#define GLAKV_LRUCACHE_H

#include "config.h"

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
    LRUCache();
    LRUCache(uint64_t capacity_in);

    bool put(uint32_t key, string &val);
    bool get(uint32_t key, string &val);
    void del(uint32_t key);
};
#endif //GLAKV_LRUCACHE_H
