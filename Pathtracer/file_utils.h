#ifndef PATHTRACER_FILE_UTILS_H
#define PATHTRACER_FILE_UTILS_H

#include "constants.h"

#include <vector>
#include <fstream>
#include <glm/glm.hpp>

namespace utils {
    void m2d_write(std::vector<glm::vec3> &data, uint x, uint y, glm::vec3 val);

    glm::vec3 m2d_read(const std::vector<glm::vec3> &data, uint x, uint y);

    inline float srgb(float linear);

    void write_ppm(const std::vector<glm::vec3> &film);
}

#endif //PATHTRACER_FILE_UTILS_H
