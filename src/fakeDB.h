//
// Created by Jiamin Huang on 5/17/16.
//

#ifndef GLAKV_FAKEDB_H
#define GLAKV_FAKEDB_H

#include "DB.h"
#include "lrucache.h"

#include <vector>

using std::vector;

class fakeDB : public DB {
private:
    shared_mutex mutex;
    uint32_t db_size;
    LRUCache cache;
    uint32_t num_prefetch;
public:
    fakeDB(string dir, uint32_t num_prefetch);
    virtual bool get(uint32_t key, string &val);
    virtual void put(uint32_t key, string &val);
    virtual void del(uint32_t key);
    virtual uint32_t size();
};


#endif //GLAKV_FAKEDB_H
