#ifndef RADIOSITY_SHARED_H
#define RADIOSITY_SHARED_H

#include <glm/glm.hpp>
#include <vector>

struct patch {
    glm::vec3 vertices[4];
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec3 rad;
    glm::vec3 rad_new;
    glm::vec3 emit;
    float area;
};

struct ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

struct settings {
    int WINDOW_WIDTH;
    int WINDOW_HEIGHT;
    float ERR;
    float FOV;
    float ASPECT_RATIO;
    int RAD_ITERATIONS;
    int FF_SAMPLES;
    glm::vec3 camera_pos;
};

#endif //RADIOSITY_SHARED_H
