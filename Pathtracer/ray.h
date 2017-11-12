#ifndef PATHTRACER_RAY_H
#define PATHTRACER_RAY_H

#include <glm/vec3.hpp>

namespace utils {
    struct ray {
        glm::vec3 origin;
        glm::vec3 direction;
    };
}

#endif //PATHTRACER_RAY_H
