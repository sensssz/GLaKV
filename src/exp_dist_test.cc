//
// Created by Jiamin Huang on 5/14/16.
//

#include "config.h"
#include "exponential_distribution.h"

#include <iostream>
#include <string>
#include <map>
#include <iomanip>
#include <map>

using std::cout;
using std::endl;
using std::string;
using std::map;

int main(int argc, char *argv[]) {
    double lambda = 1;
    if (argc == 2) {
        lambda = atof(argv[1]);
    }
    map<uint64_t, uint64_t> hist;
    exponential_distribution distribution(lambda, 100);

    for (uint64_t count = 0; count < 100000; ++count) {
        uint64_t val = distribution.next();
        hist[val]++;
    }

    for (auto iter : hist) {
        if (iter.second / 1000 > 0) {
            string val(iter.second / 100, '*');
            cout << iter.first << ": " << val << endl;
        }
    }

    return 0;
}