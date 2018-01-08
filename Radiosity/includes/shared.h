#ifndef RADIOSITY_SHARED_H
#define RADIOSITY_SHARED_H

#define GLEW_STATIC

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include <thread>
#include <atomic>

const float INF = std::numeric_limits<float>::infinity();

struct patch {
    glm::vec3 vertices[4];
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec3 rad;
    glm::vec3 rad_new;
    glm::vec3 emit;
    float area;
    glm::vec3 p_total;
    glm::vec3 p_unshot;
    glm::vec3 p_recieved;
    glm::vec3 colors[4];
    void *parent;
};

struct hit {
    bool hit;
    float t;
    patch *p;
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
    long TOTAL_RAYS;
    int GATHER_RAYS;
    int SHADOW_RAYS;
    glm::vec3 camera_pos;
    std::string mesh_path;
};

float intersect(const ray &r, const patch &p, float ERR);

#endif //RADIOSITY_SHARED_H
