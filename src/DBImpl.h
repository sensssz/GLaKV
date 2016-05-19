//
// Created by Jiamin Huang on 5/14/16.
//

#ifndef GLAKV_DB_IMPL_H
#define GLAKV_DB_IMPL_H

#include "config.h"
#include "DB.h"
#include "lrucache.h"

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
