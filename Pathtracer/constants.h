#ifndef PATHTRACER_CONSTANTS_H
#define PATHTRACER_CONSTANTS_H

#include <glm/vec3.hpp>
#include <random>

namespace constants {
    /* Picture constants */
    const unsigned int max_colors = 255;
    const float window_width = 512.0f;
    const float window_height = 512.0f;

    /* Samples (tweak this to change quality) */
    const unsigned int max_bounces = 8;
    const unsigned int spp = 256;
    const float inv_spp = 1.0f / static_cast<float>(spp);

    /* Math */
    const float pi = 3.1415926f;
    const float inv_pi = 1.0f / pi;

    /* World description */
    const glm::vec3 world_up(0.0f, 0.0f, 1.0f);
    const float error = 1e-5;
}

#endif //PATHTRACER_CONSTANTS_H
