#include <iostream>
#include <iomanip>

#include "../includes/stats.h"

void output_stats(stats &stat) {
    std::cout << "[=========STATS=========]" << std::endl;
    std::cout << "| " << std::right << std::setw(15) << "POLYGONS: "
              << std::left << stat.polygons_count << std::endl;
    std::cout << "| " << std::right << std::setw(15) << "LIGHTS: "
              << std::left << stat.light_sources_count << std::endl;
    std::cout << "| " << std::right << std::setw(15) << "RAYS: "
              << std::left << stat.rays_number << std::endl;

    std::cout << "[=======================]" << std::endl;

    std::cout << "| " << std::right << std::setw(15) << "PRE-INIT: "
              << std::left << std::setprecision(5)
              << (stat.events[EVENT::MESH_BEGIN] - stat.events[EVENT::STARTUP]) * 1000.0
              << "ms" << std::endl;
    std::cout << "| " << std::right << std::setw(15) << "MESH LOADING: "
              << std::left << std::setprecision(5)
              << (stat.events[EVENT::MESH_END] - stat.events[EVENT::MESH_BEGIN]) * 1000.0
              << "ms" << std::endl;
    std::cout << "| " << std::right << std::setw(15) << "BVH: "
              << std::left << std::setprecision(5)
              << (stat.events[EVENT::BVH_END] - stat.events[EVENT::BVH_BEGIN]) * 1000.0f
              << "ms" << std::endl;

    double rad_time = stat.events[EVENT::SIJIA_END] - stat.events[EVENT::SIJIA_BEGIN];

    std::cout << "| " << std::right << std::setw(15) << "RADIOSITY: "
              << std::left << std::setprecision(5)
              << rad_time << "sec" << std::endl;

    std::cout << "| " << std::right << std::setw(15) << "PER-RAY: "
              << std::left << std::setprecision(3)
              << rad_time * 1000.0 / stat.rays_number << "ms" << std::endl;

    std::cout << "| " << std::right << std::setw(15) << "TONEMAPPING: "
              << std::left << std::setprecision(2)
              << (stat.events[EVENT::TONEMAP_END] - stat.events[EVENT::TONEMAP_BEGIN]) * 1000.0
              << "ms" << std::endl;
    std::cout << "[=======================]" << std::endl;
}
