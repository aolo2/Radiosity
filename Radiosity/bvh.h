#ifndef RADIOSITY_BVH_H
#define RADIOSITY_BVH_H

#include "shared.h"

#ifdef DEBUG
const int MAX_TREE_DEPTH = 8;
const int MIN_PATCH_PER_LEAF = 5;
const float INF = std::numeric_limits<float>::infinity();
#endif

const int B_PLANES = 7;
const float s3o3 = glm::sqrt(3.0f) / 3.0f;

struct bbox {
    glm::vec3 normals[B_PLANES] = {
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(s3o3, s3o3, s3o3),
            glm::vec3(-1.0f * s3o3, s3o3, s3o3),
            glm::vec3(-1.0f * s3o3, -1.0f * s3o3, s3o3),
            glm::vec3(s3o3, -1.0f * s3o3, s3o3),
    };

    float d_near[B_PLANES];
    float d_far[B_PLANES];
};

#ifdef DEBUG
const glm::vec3 axis[3] = {
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
};

struct aabb {
    float min[3]; // AABB (x,y,z)
    float max[3];
};

struct node {
    aabb aabbox;
    std::vector<node> children;
    bool is_leaf = false;
    std::vector<patch> patches;
};

struct tree {
    aabb aabbox;
    node nodes[2]; // nodes[0].x|y|z < noddes[1].x|y|z
};

tree compute_tree(std::vector<patch> patches);

float intersect(const ray &r, const tree &T, float ERR);

float intersect(const ray &r, const patch &p, float ERR); // implemented in rad.cpp

#endif

struct object {
    std::vector<patch> patches;
    std::string name;
#ifdef DEBUG
    tree root;
#endif
    bbox box;
};

bbox compute_box(const std::vector<patch> &patches);

bool intersect(const ray &r, const bbox &boxm, float ERR);

#endif //RADIOSITY_BVH_H