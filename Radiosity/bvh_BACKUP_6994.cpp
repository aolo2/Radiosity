#include "bvh.h"

#include <algorithm>

aabb compute_box(const std::vector<patch> &patches) {
    aabb box = {};

<<<<<<< HEAD
    std::fill_n(box.d_near, B_PLANES, INF);
    std::fill_n(box.d_far, B_PLANES, INF * -1.0f);

    for (const auto &p : patches) {
        for (int i = 0; i < 3; i++) { // note, we do not include the centroid
            for (int k = 0; k < B_PLANES; k++) {
                glm::vec3 N = box.normals[k];
                float D = glm::dot(N, p.vertices[i]);

                box.d_near[k] = std::min(D, box.d_near[k]);
                box.d_far[k] = std::max(D, box.d_far[k]);
=======
    box.near = glm::vec3(INF, INF, INF);
    box.far = -1.0f * box.near;

    for (const auto &p : patches) {
        for (int i = 0; i < b_planes; i++) {
            for (int j = 0; j < 3; j++) {
                box.near[i] = std::min(p.vertices[j][i], box.near[i]);
                box.far[i] = std::max(p.vertices[j][i], box.far[i]);
>>>>>>> bvh-dev
            }
        }
    }

    return box;
}

<<<<<<< HEAD
bool intersect(const ray &r, const bbox &box, float ERR) {
    float t_near_max = INF * -1.0f;
    float t_far_min = INF;

    for (int i = 0; i < B_PLANES; i++) {
        float NdotR = glm::dot(box.normals[i], r.direction);
        if (glm::abs(NdotR) < ERR) { return false; }
        float NdotO = glm::dot(box.normals[i], r.origin);
=======
bool intersect(const ray &r, const aabb &box, float ERR) {
    float t_near_max = INF * -1.0f;
    float t_far_min = INF;

    for (int i = 0; i < b_planes; i++) {
>>>>>>> bvh-dev

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

<<<<<<< HEAD
#ifdef DEBUG

float min_patch_x(const patch &p) {
    float res = INF;
    for (int i = 0; i < 3; i++) {
        res = std::min(p.vertices[i].x, res);
    }

    return res;
}

float max_patch_x(const patch &p) {
    float res = INF * -1.0f;
    for (int i = 0; i < 3; i++) {
        res = std::max(p.vertices[i].x, res);
    }

    return res;
}

node create_node(float min_x, float max_x, std::vector<patch> &&patches) {
    node res = {};

    res.aabbox.min[0] = min_x;
    res.aabbox.max[0] = max_x;

    if (patches.size() <= MIN_PATCH_PER_LEAF) {
        res.is_leaf = true;
        res.patches = patches;
        return res;
    } else {
        auto middle = patches[patches.size() / 2];
        auto middle_next = patches[patches.size() / 2 + 1];

        res.children.push_back(create_node(min_x,
                                           max_patch_x(middle),
                                           std::vector<patch>(patches.begin(), patches.begin() + patches.size() / 2)));
        res.children.push_back(create_node(min_patch_x(middle_next),
                                           max_x,
                                           std::vector<patch>(patches.begin(), patches.begin() + patches.size() / 2)));
    }

    return res;
}

tree compute_tree(std::vector<patch> patches) {
    tree res = {};

    std::fill_n(res.aabbox.min, 3, INF);
    std::fill_n(res.aabbox.max, 3, INF * -1.0f);

    /* Compute root AABB */
    for (const auto &p : patches) {
        for (int i = 0; i < 3; i++) { // axis
            for (int k = 0; k < 3; k++) { // vertices of a patch
                res.aabbox.min[i] = std::min(p.vertices[k][i], res.aabbox.min[i]);
                res.aabbox.max[i] = std::max(p.vertices[k][i], res.aabbox.max[i]);
            }
        }
    }

    /* Sort by x (ascending) */
    std::sort(patches.begin(), patches.end(), [](const patch &p1, const patch &p2) -> bool {
        float min_x1 = INF;
        float min_x2 = INF;

        for (int i = 0; i < 3; i++) {
            min_x1 = std::min(p1.vertices[i].x, min_x1);
            min_x2 = std::min(p2.vertices[i].x, min_x2);
        }

        return min_x1 < min_x2;
    });

    // FIXME: what if there is only 1 triangle?
    auto middle = patches[patches.size() / 2];
    auto middle_next = patches[patches.size() / 2 + 1];

    res.nodes[0] = create_node(res.aabbox.min[0],
                               max_patch_x(middle),
                               std::vector<patch>(patches.begin(), patches.begin() + patches.size() / 2));
    res.nodes[1] = create_node(min_patch_x(middle_next),
                               res.aabbox.max[0],
                               std::vector<patch>(patches.begin() + patches.size() / 2, patches.end()));

    /* Free all non-leaf patch vectors */
    // ...

    return res;
}

float intersect(const ray &r, const aabb &box, float ERR) {
    float t_near_max = INF * -1.0f;
    float t_far_min = INF;

    for (int i = 0; i < 3; i++) {
        float NdotR = glm::dot(axis[i], r.direction);
        if (glm::abs(NdotR) == 0.0f) { return -1.0f; }
        float NdotO = glm::dot(axis[i], r.origin);

        float t_near = (box.min[i] - NdotO) / NdotR;
        float t_far = (box.max[i] - NdotO) / NdotR;

        if (NdotR < 0.0f) {
            std::swap(t_near, t_far);
        }

        t_near_max = std::max(t_near, t_near_max);
        t_far_min = std::min(t_far, t_far_min);
    }

    if (t_far_min - t_near_max >= -ERR) {
        return t_near_max;
    } else {
        return -1.0f;
    }
}

float intersect(const ray &r, const node &N, const glm::vec3 &hit, float ERR) {
    float t = intersect(r, N.aabbox, ERR);

    if (t < -ERR) {
        return -1.0f;
    }

    if (!N.is_leaf) {
        if (hit.x >= N.children[0].aabbox.min[0] &&
            hit.x <= N.children[0].aabbox.min[0]) {

            float t_node = intersect(r, N.children[0], hit, ERR);
            if (t_node > ERR && t_node < t) {
                return t_node;
            }
        } else {
            float t_node = intersect(r, N.children[1], hit, ERR);
            if (t_node > ERR && t_node < t) {
                return t_node;
            }
        }
    } else {
        float t_patch_min = INF;

        for (const auto &p : N.patches) {
            float t_patch = intersect(r, p, ERR);
            if (t_patch > ERR) {
                t_patch_min = std::min(t_patch, t_patch_min);
            }
        }

        return (t_patch_min != INF) ? t_patch_min : -1.0f;
    }
}

float intersect(const ray &r, const tree &T, float ERR) {
    float t = intersect(r, T.aabbox, ERR);

    if (t < -ERR) {
        return -1.0f;
    }

    glm::vec3 hit = r.origin + t * r.direction;

    if (hit.x >= T.nodes[0].aabbox.min[0] &&
        hit.x <= T.nodes[0].aabbox.max[0]) {

        float t_node = intersect(r, T.nodes[0], hit, ERR);
        if (t_node > ERR && t_node < t) {
            return t_node;
        }
    } else {
        float t_node = intersect(r, T.nodes[1], hit, ERR);
        if (t_node > ERR && t_node < t) {
            return t_node;
        }
    }

    return -1.0f;
}

#endif
=======
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
                    const std::vector<patch *> &primitives) {

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

float intersect(const ray &r, const bvh_node *node,
                const std::vector<patch *> &primitives, float ERR) {
    float t = INF;

    if (!intersect(r, node->box, ERR)) {
        return -1.0f;
    }

    if (node->split == axis::none) {
        for (int i = 0; i < node->prim_num; i++) {
            float t_now = intersect(r, *primitives[node->prim_base + i], ERR);
            if (t_now > 0.0f) { t = std::min(t, t_now); }
        }
    } else {
        auto t_c0 = intersect(r, node->children[0], primitives, ERR);
        auto t_c1 = intersect(r, node->children[1], primitives, ERR);

        if (t_c0 > ERR) {
            t = std::min(t, t_c0);
        }

        if (t_c1 > ERR) {
            t = std::min(t, t_c1);
        }
    }

    return t;
}

#ifdef DEBUG

std::vector<float> box_vertices(const aabb &box, int depth) {
    glm::vec3 color(1.0f, 0.0f, 0.0f);

    if (depth % 3 == 2) {
        color = glm::vec3(0.0f, 1.0f, 0.0f);
    } else if (depth % 3 == 0) {
        color = glm::vec3(1.0f, 0.0f, 1.0f);
    }

    return {
            // face 1
            box.near[0], box.near[1], box.near[2], color.r, color.g, color.b,
            box.far[0], box.far[1], box.near[2], color.r, color.g, color.b,
            box.far[0], box.near[1], box.near[2], color.r, color.g, color.b,
            box.far[0], box.far[1], box.near[2], color.r, color.g, color.b,
            box.near[0], box.near[1], box.near[2], color.r, color.g, color.b,
            box.near[0], box.far[1], box.near[2], color.r, color.g, color.b,
            // end face 1

            // face 2
            box.near[0], box.near[1], box.far[2], color.r, color.g, color.b,
            box.far[0], box.near[1], box.far[2], color.r, color.g, color.b,
            box.far[0], box.far[1], box.far[2], color.r, color.g, color.b,
            box.far[0], box.far[1], box.far[2], color.r, color.g, color.b,
            box.near[0], box.far[1], box.far[2], color.r, color.g, color.b,
            box.near[0], box.near[1], box.far[2], color.r, color.g, color.b,
            // end face 2

            // face 3
            box.near[0], box.far[1], box.far[2], color.r, color.g, color.b,
            box.near[0], box.far[1], box.near[2], color.r, color.g, color.b,
            box.near[0], box.near[1], box.near[2], color.r, color.g, color.b,
            box.near[0], box.near[1], box.near[2], color.r, color.g, color.b,
            box.near[0], box.near[1], box.far[2], color.r, color.g, color.b,
            box.near[0], box.far[1], box.far[2], color.r, color.g, color.b,
            // end face 3

            // face 4
            box.far[0], box.far[1], box.far[2], color.r, color.g, color.b,
            box.far[0], box.near[1], box.near[2], color.r, color.g, color.b,
            box.far[0], box.far[1], box.near[2], color.r, color.g, color.b,
            box.far[0], box.near[1], box.near[2], color.r, color.g, color.b,
            box.far[0], box.far[1], box.far[2], color.r, color.g, color.b,
            box.far[0], box.near[1], box.far[2], color.r, color.g, color.b,
            // end face 4

            // face 5
            box.near[0], box.near[1], box.near[2], color.r, color.g, color.b,
            box.far[0], box.near[1], box.near[2], color.r, color.g, color.b,
            box.far[0], box.near[1], box.far[2], color.r, color.g, color.b,
            box.far[0], box.near[1], box.far[2], color.r, color.g, color.b,
            box.near[0], box.near[1], box.far[2], color.r, color.g, color.b,
            box.near[0], box.near[1], box.near[2], color.r, color.g, color.b,
            // end face 5

            // face 6
            box.near[0], box.far[1], box.near[2], color.r, color.g, color.b,
            box.far[0], box.far[1], box.far[2], color.r, color.g, color.b,
            box.far[0], box.far[1], box.near[2], color.r, color.g, color.b,
            box.far[0], box.far[1], box.far[2], color.r, color.g, color.b,
            box.near[0], box.far[1], box.near[2], color.r, color.g, color.b,
            box.near[0], box.far[1], box.far[2], color.r, color.g, color.b,
            // end face 6
    };
}

std::vector<float> bvh_debug_vertices(const bvh_node *node, const int depth) {
    std::vector<float> res;

    if (depth == MAX_DEPTH) {
        return res;
    }

    auto box_v = box_vertices(node->box, depth);
    res.insert(res.end(), box_v.begin(), box_v.end());

    if (node->children[0] != nullptr) {
        auto c0_v = bvh_debug_vertices(node->children[0], depth + 1);
        res.insert(res.end(), c0_v.begin(), c0_v.end());
    }

    if (node->children[1] != nullptr) {
        auto c1_v = bvh_debug_vertices(node->children[1], depth + 1);
        res.insert(res.end(), c1_v.begin(), c1_v.end());
    }

    return res;
}
#endif
>>>>>>> bvh-dev
