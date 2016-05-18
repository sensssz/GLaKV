//
// Created by Jiamin Huang on 5/17/16.
//

#ifndef GLAKV_DB_H
#define GLAKV_DB_H

#include <cstdint>
#include <string>

using std::string;

class DB {
public:
    virtual bool get(uint32_t key, string &val) = 0;
    virtual void put(uint32_t key, string &val) = 0;
    virtual void del(uint32_t key) = 0;
    virtual uint32_t size() = 0;
};

#endif //GLAKV_DB_H
