//
// Created by Jiamin Huang on 5/14/16.
//

#include "config.h"
#include "exponential_distribution.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <map>

using std::cout;
using std::endl;
using std::string;
using std::unordered_map;

int main(int argc, char *argv[]) {
    double lambda = 1;
    if (argc == 2) {
        lambda = atof(argv[1]);
    }
    unordered_map<uint64_t, uint64_t> map;
    exponential_distribution distribution(lambda, DB_SIZE);

    for (uint64_t count = 0; count < 100000; ++count) {
        uint64_t val = distribution.next();
        map[val]++;
    }

    for (auto iter : map) {
        string val(map[iter.second] / 100, '*');
        cout << iter.first << ": " << val << endl;
    }

    return 0;
}