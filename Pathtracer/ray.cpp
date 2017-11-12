//
// Created by aolo2 on 11/12/17.
//

#include <glm/glm.hpp>
#include <iostream>
#include "ray.h"
#include "constants.h"


// NOTE: implementation comes from WIKI
float intersect_triangle(const ray &ray, const triangle &triangle) {
    glm::vec3 e1 = triangle.b - triangle.a;
    glm::vec3 e2 = triangle.c - triangle.a;

    glm::vec3 pvec = glm::cross(ray.direction, e2);
    float det = glm::dot(e1, pvec);

    if (glm::abs(det) < constants::error) {
        return 0.0f;
    }

    float inv_det = 1.0f / det;
    glm::vec3 tvec = ray.origin - triangle.a;
    float u = glm::dot(tvec, pvec) * inv_det;

    if (u < 0.0f || u > 1.0f) {
        return 0.0f;
    }

    glm::vec3 qvec = glm::cross(tvec, e1);
    float v = glm::dot(ray.direction, qvec) * inv_det;

    if (v < 0.0f || u + v > 1.0f) {
        return 0.0f;
    }

    return glm::dot(e2, qvec) * inv_det;
}
