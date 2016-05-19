//
// Created by Jiamin Huang on 5/13/16.
//

#include "exponential_distribution.h"

#include <iostream>

using std::cout;
using std::endl;

using std::ceil;

static const double LN10 = 2.30258509299;
static const int PRECISION = 4;

exponential_distribution::exponential_distribution(double lambda_in, uint32_t size_in)
        : dist((lambda_in == 0) ? 0 : 1.0 / lambda_in), lambda(lambda_in), size(size_in) {
    std::random_device rd;
    generator.seed(rd());
    max_val = (lambda == 0) ? 0 : (PRECISION / lambda * LN10);
    interval_size = max_val / size;
}

uint32_t exponential_distribution::next() {
    uint32_t key = 0;
    if (lambda > 0) {
        double rand;
        do {
            rand = dist(generator);
            key = (uint32_t) (rand / interval_size);
        } while (rand > max_val);
    }
    return key;
}

