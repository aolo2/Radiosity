#ifndef RADIOSITY_TIMER_H
#define RADIOSITY_TIMER_H

#include <map>

enum EVENT {
    STARTUP,
    MESH_BEGIN,
    MESH_END,
    BVH_BEGIN,
    BVH_END,
    SIJIA_BEGIN,
    SIJIA_END,
    TONEMAP_BEGIN,
    TONEMAP_END
};

struct stats {
    std::map<EVENT, double> events;
    long long light_sources_count;
    long long rays_number;
    long long polygons_count;
    long long iterations_number;
};

void output_stats(stats &stat);

#endif //RADIOSITY_TIMER_H
