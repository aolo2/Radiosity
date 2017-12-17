#include "bvh.h"

#include <algorithm>

bbox compute_box(const std::vector<patch> &patches) {
    bbox box = {};

    std::fill_n(box.d_near, b_planes, std::numeric_limits<float>::infinity());
    std::fill_n(box.d_far, b_planes, std::numeric_limits<float>::infinity() * -1.0f);

    for (const auto &p : patches) {
        for (int i = 0; i < 3; i++) { // note, we do not include the centroid
            for (int k = 0; k < b_planes; k++) {
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
    float t_near_max = std::numeric_limits<float>::infinity() * -1.0f;
    float t_far_min = std::numeric_limits<float>::infinity();

    for (int i = 0; i < b_planes; i++) {
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


