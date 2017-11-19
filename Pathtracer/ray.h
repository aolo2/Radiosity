#ifndef PATHTRACER_RAY_H
#define PATHTRACER_RAY_H

#include <glm/vec3.hpp>

struct ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

struct triangle {
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;
    glm::vec3 normal;
};

struct point {
    glm::vec3 position;
    glm::vec3 normal;
    unsigned int materialID;
};

struct plane {
    glm::vec3 normal;
    float d;
};

struct sphere {
    glm::vec3 position;
    float r;
};

float intersect_triangle(const ray &ray, const triangle &triangle);

#endif //PATHTRACER_RAY_H
