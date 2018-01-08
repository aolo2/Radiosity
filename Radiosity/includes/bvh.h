#ifndef RADIOSITY_BVH_H
#define RADIOSITY_BVH_H

#include "shared.h"

const int b_planes = 3;

enum axis {
    x = 0,
    y = 1,
    z = 2,
    none = 3,
};

struct aabb {
    glm::vec3 near;
    glm::vec3 far;
    // todo need to only store length of diagonal
};

struct bvh_node {
    aabb box;
    bvh_node *children[2];
    axis split;
    std::size_t prim_base;
    std::size_t prim_num;
    bvh_node *parent = nullptr;
};

struct prim_info {
    std::size_t prim_idx;
    aabb box;
    glm::vec3 centroid;
};

const int MAX_DEPTH = 8;

aabb compute_box(const std::vector<patch> &patches);

bool intersect(const ray &r, const aabb &box, float ERR);

bvh_node *bvh(std::vector<patch *> &primitives);

hit intersect(const ray &r, const bvh_node *node,
              const std::vector<patch *> &primitives, float ERR);

std::vector<float> bvh_debug_vertices(const bvh_node *node, int depth);

#endif //RADIOSITY_BVH_H
