#ifndef RADIOSITY_SHARED_H
#define RADIOSITY_SHARED_H

#define GLEW_STATIC
//#define DEBUG       // DEBUG VOLUMES FOR BVH
#define LOCAL       // LOCAL-LINE RADIOSITY

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <vector>
#include <string>

const float INF = std::numeric_limits<float>::infinity();

struct patch {
    glm::vec3 vertices[4];
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec3 rad;
    glm::vec3 rad_new;
    glm::vec3 emit;
    float area;
#ifdef LOCAL
    float p_total;
    float p_unshot;
    float p_recieved;
#endif
};

#ifdef LOCAL
struct hit {
    bool hit;
    float t;
    patch *p;
};
#endif

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
    std::string mesh_path;
};

float intersect(const ray &r, const patch &p, float ERR);

#endif //RADIOSITY_SHARED_H
