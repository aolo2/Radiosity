#include "bvh.h"

#include <algorithm>

bbox compute_box(const std::vector<patch> &patches) {
    bbox box = {};

    std::fill_n(box.d_near, B_PLANES, INF);
    std::fill_n(box.d_far, B_PLANES, INF * -1.0f);

    for (const auto &p : patches) {
        for (int i = 0; i < 3; i++) { // note, we do not include the centroid
            for (int k = 0; k < B_PLANES; k++) {
                glm::vec3 N = box.normals[k];
                float D = glm::dot(N, p.vertices[i]);

                box.d_near[k] = std::min(D, box.d_near[k]);
                box.d_far[k] = std::max(D, box.d_far[k]);
            }
        }
    }

    return box;
}

bool intersect(const ray &r, const bbox &box, float ERR) {
    float t_near_max = INF * -1.0f;
    float t_far_min = INF;

    for (int i = 0; i < B_PLANES; i++) {
        float NdotR = glm::dot(box.normals[i], r.direction);
        if (glm::abs(NdotR) < ERR) { return false; }
        float NdotO = glm::dot(box.normals[i], r.origin);

        float t_near = (box.d_near[i] - NdotO) / NdotR;
        float t_far = (box.d_far[i] - NdotO) / NdotR;

        if (NdotR < 0.0f) {
            std::swap(t_near, t_far);
        }

        t_near_max = std::max(t_near, t_near_max);
        t_far_min = std::min(t_far, t_far_min);
    }

    return t_far_min >= t_near_max;
}

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
