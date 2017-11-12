#ifndef PATHTRACER_SAMPLER_H
#define PATHTRACER_SAMPLER_H

#include <random>

// FIXME(aolo2): need non-static init for samplers
namespace utils {
    /* Sampling */
    std::random_device rd;
    std::mt19937 mt(rd());
}


#endif //PATHTRACER_SAMPLER_H
