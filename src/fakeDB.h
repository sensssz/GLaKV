//
// Created by Jiamin Huang on 5/17/16.
//

#ifndef GLAKV_FAKEDB_H
#define GLAKV_FAKEDB_H

#include "DB.h"

class fakeDB : public DB {
public:
    virtual bool get(uint32_t key, string &val);
    virtual void put(uint32_t key, string &val);
    virtual void del(uint32_t key);
    virtual uint32_t size();
};


#endif //GLAKV_FAKEDB_H
