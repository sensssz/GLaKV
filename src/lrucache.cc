//
// Created by Jiamin Huang on 5/19/16.
//

#include "lrucache.h"

using boost::upgrade_lock;
using boost::upgrade_to_unique_lock;

LRUCache::LRUCache() : LRUCache(CACHE_SIZE) {}

LRUCache::LRUCache(uint64_t capacity_in) : capacity(capacity_in), size(0) {}

bool LRUCache::put(uint32_t key, string &val) {
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

bool LRUCache::get(uint32_t key, string &val) {
    unique_lock<shared_mutex> lock(mutex);
    auto map_iter = cache.find(key);
    if (map_iter != cache.end()) {
        val = map_iter->second.first;
        lru_keys.erase(map_iter->second.second);
        lru_keys.push_front(key);
        map_iter->second.second = lru_keys.begin();
        return true;
    }
    return false;
}

void LRUCache::del(uint32_t key) {
    unique_lock<shared_mutex> lock(mutex);
    auto map_iter = cache.find(key);
    if (map_iter != cache.end()) {
        lru_keys.erase(map_iter->second.second);
        cache.erase(key);
        --size;
    }
}