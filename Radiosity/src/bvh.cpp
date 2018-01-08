#include "../includes/bvh.h"

#include <algorithm>

std::vector<patch *> patches(const std::vector<patch *> &primitives, bvh_node *node) {
    std::vector<patch *> res;

    if (node->prim_num > 0) {
        for (int i = 0; i < node->prim_num; i++) {
            res.push_back(primitives[node->prim_base + i]);
        }
    }

    if (node->split != axis::none) {
        if (node->children[0] != nullptr) {
            auto c0_res = patches(primitives, node->children[0]);
            res.insert(res.end(), c0_res.begin(), c0_res.end());
        }

        if (node->children[1] != nullptr) {
            auto c1_res = patches(primitives, node->children[1]);
            res.insert(res.end(), c1_res.begin(), c1_res.end());
        }
    }

    return res;
}

std::vector<patch *> neighbors(const std::vector<patch *> &primitives, const patch &p, bvh_node *tree) {
    std::vector<patch *> res;

    auto *node = (bvh_node *) p.parent;
    const int TRACEBACK_LEN = 5;

    for (int i = 0; i < TRACEBACK_LEN && node != nullptr; i++) {
        node = node->parent;
    }

    return patches(primitives, node);
}

aabb compute_box(const std::vector<patch> &patches) {
    aabb box = {};

    box.near = glm::vec3(INF, INF, INF);
    box.far = -1.0f * box.near;

    for (const auto &p : patches) {
        for (int i = 0; i < b_planes; i++) {
            for (int j = 0; j < 3; j++) {
                box.near[i] = std::min(p.vertices[j][i], box.near[i]);
                box.far[i] = std::max(p.vertices[j][i], box.far[i]);
            }
        }
    }

    return box;
}

bool intersect(const ray &r, const aabb &box, float ERR) {
    float t_near_max = INF * -1.0f;
    float t_far_min = INF;

    for (int i = 0; i < b_planes; i++) {

        float dir_component = r.direction[i];
        float orig_component = r.origin[i];

        if (glm::abs(dir_component) < ERR) { return false; }

        float t_near = (box.near[i] - orig_component) / dir_component;
        float t_far = (box.far[i] - orig_component) / dir_component;

        if (dir_component < ERR) {
            std::swap(t_near, t_far);
        }

        t_near_max = std::max(t_near, t_near_max);
        t_far_min = std::min(t_far, t_far_min);
    }

    return t_far_min >= t_near_max;
}

aabb join(const aabb &b1, const aabb &b2) {
    aabb res = {};

    for (int i = 0; i < 3; i++) {
        res.near[i] = std::min(b1.near[i], b2.near[i]);
        res.far[i] = std::max(b1.far[i], b2.far[i]);
    }

    return res;
}

aabb join(const aabb &b, const glm::vec3 &p) {
    aabb box = b;

    for (int i = 0; i < 3; i++) {
        box.far[i] = std::max(box.far[i], p[i]);
        box.near[i] = std::min(box.near[i], p[i]);
    }

    return box;
}

void init_leaf(bvh_node *leaf, std::size_t first, std::size_t n, const aabb &box) {
    leaf->box = box;
    leaf->prim_base = first;
    leaf->prim_num = n;
    leaf->children[0] = leaf->children[1] = nullptr;
    leaf->split = axis::none;
}

void init_interior(bvh_node *node, axis split, bvh_node *c0, bvh_node *c1) {
    node->split = split;
    node->children[0] = c0;
    node->children[1] = c1;
    node->prim_num = 0;
    node->box = join(c0->box, c1->box);

    /* For bottom-to-top traverse */
    c0->parent = c1->parent = node;
}

axis max_extent(const aabb &box) {

    glm::vec3 extent = box.far - box.near;

    if (extent.x > extent.y) {
        return (extent.x > extent.z) ? axis::x : axis::z;
    } else {
        return (extent.y > extent.z) ? axis::y : axis::z;
    }
}

bvh_node *rec_build(std::vector<prim_info> &primitive_info, std::size_t start,
                    std::size_t end, std::vector<patch *> &ordered_primititves,
                    std::vector<patch *> &primitives) {

    bvh_node *node = (bvh_node *) malloc(sizeof(bvh_node));

    aabb bounds = primitive_info[start].box;
    for (auto i = start; i < end; i++) {
        bounds = join(bounds, primitive_info[i].box);
    }

    std::size_t prim_count = end - start;
    if (prim_count == 1) {
        auto first_offset = ordered_primititves.size();
        for (auto i = start; i < end; i++) {
            auto prim_num = primitive_info[i].prim_idx;
            ordered_primititves.push_back(primitives[prim_num]);
        }
        init_leaf(node, first_offset, prim_count, bounds);

        /* So that we can get node from patch directly */
        for (int i = 0; i < prim_count; i++) {
            primitives[first_offset + i]->parent = (void *) node;
        }

        return node;
    } else {
        aabb centroid_box = {};
        centroid_box.near = glm::vec3(INF);
        centroid_box.far = glm::vec3(INF * -1.0f);

        for (auto i = start; i < end; i++) {
            centroid_box = join(centroid_box, primitive_info[i].centroid);
        }

        axis dim = max_extent(centroid_box);
        auto mid = (start + end) / 2;

        if (centroid_box.far[dim] == centroid_box.near[dim]) {
            // TODO: вынести функцию
            auto first_offset = ordered_primititves.size();
            for (auto i = start; i < end; i++) {
                auto prim_num = primitive_info[i].prim_idx;
                ordered_primititves.push_back(primitives[prim_num]);
            }
            init_leaf(node, first_offset, prim_count, bounds);

            /* So that we can get node from patch directly */
            for (int i = 0; i < prim_count; i++) {
                primitives[first_offset + i]->parent = (void *) node;
            }

            return node;
        } else {
            // for now
            mid = (start + end) / 2;
            std::nth_element(&primitive_info[start], &primitive_info[mid], &primitive_info[end - 1] + 1,
                             [dim](prim_info &a, prim_info &b) -> bool {
                                 return a.centroid[dim] < b.centroid[dim];
                             });
            // end for now

            init_interior(node, dim,
                          rec_build(primitive_info, start, mid, ordered_primititves, primitives),
                          rec_build(primitive_info, mid, end, ordered_primititves, primitives));
        }
    }

    return node;
}

bvh_node *bvh(std::vector<patch *> &primitives) {
    /* 1. Bounding volumes for each primitive */
    std::vector<prim_info> primitive_info(primitives.size());
    std::vector<patch *> ordered_primititves;

    for (std::size_t i = 0; i < primitives.size(); i++) {
        auto box = compute_box({*primitives[i]});
        primitive_info[i] = {i, box, (box.near + box.far) / 2.0f};
    }

    /* 2. Construct the BVH */
    bvh_node *root = rec_build(primitive_info, 0, primitives.size(), ordered_primititves, primitives);
    std::swap(ordered_primititves, primitives);

    /* 3. Convert to compact */
    // TODO

    return root;
}

hit intersect(const ray &r, const bvh_node *node,
              const std::vector<patch *> &primitives, float ERR) {

    if (!intersect(r, node->box, ERR)) {
        hit res = {};
        res.hit = false;
        return res;
    }

    hit ret = {};
    ret.hit = false;
    ret.t = INF;

    if (node->split == axis::none) {
        for (int i = 0; i < node->prim_num; i++) {
            float t_now = intersect(r, *primitives[node->prim_base + i], ERR);
            if (t_now > ERR) {
                if (t_now < ret.t) {
                    ret.t = t_now;
                    ret.hit = true;
                    ret.p = primitives[node->prim_base + i];
                }
            }
        }
    } else {
        auto hit_c0 = intersect(r, node->children[0], primitives, ERR);
        auto hit_c1 = intersect(r, node->children[1], primitives, ERR);

        if (hit_c0.hit && hit_c0.t > ERR) {
            if (hit_c0.t < ret.t) {
                ret.t = hit_c0.t;
                ret.p = hit_c0.p;
                ret.hit = true;
            }
        }

        if (hit_c1.hit && hit_c1.t > ERR) {
            if (hit_c1.t < ret.t) {
                ret.t = hit_c1.t;
                ret.p = hit_c1.p;
                ret.hit = true;
            }
        }
    }

    return ret;
}