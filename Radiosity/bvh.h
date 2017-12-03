#ifndef RADIOSITY_BVH_H
#define RADIOSITY_BVH_H

#include "shared.h"

const int b_planes = 7;
const float s3o3 = glm::sqrt(3.0f) / 3.0f;

struct bbox {
    glm::vec3 normals[b_planes] = {
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(s3o3, s3o3, s3o3),
            glm::vec3(-1.0f * s3o3, s3o3, s3o3),
            glm::vec3(-1.0f * s3o3, -1.0f * s3o3, s3o3),
            glm::vec3(s3o3, -1.0f * s3o3, s3o3),
    };

    float d_near[b_planes];
    float d_far[b_planes];
};

struct object {
    std::vector<patch> patches;
    std::string name;
    bbox box;
};

bbox compute_box(const std::vector<patch> &patches);

bool intersect(const ray &r, const bbox &boxm,  float ERR);

#endif //RADIOSITY_BVH_H
