#include "radiosity.h"

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_real_distribution<float> unilateral(0.0f, 1.0);

const float PI = 3.1415926f;

float area(const patch &p) {
    glm::vec3 a = p.vertices[0];
    glm::vec3 b = p.vertices[1];
    glm::vec3 c = p.vertices[2];

    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;

    float ab_len = glm::length(ab);
    float ac_len = glm::length(ac);
    float cos = glm::dot(ab, ac) / (ab_len * ac_len);

    return 0.5f * ab_len * ac_len * glm::sqrt(1 - cos * cos);
}

glm::vec3 sample_point(const patch &p) {

    float r1 = unilateral(mt);
    float r2 = unilateral(mt);

    return glm::vec3((1 - glm::sqrt(r1)) * p.vertices[0]
                     + glm::sqrt(r1) * (1 - r2) * p.vertices[1]
                     + r2 * glm::sqrt(r1) * p.vertices[2]);
}

float intersect(const ray &r, const patch &p, float ERR) {
    glm::vec3 e1 = p.vertices[1] - p.vertices[0];
    glm::vec3 e2 = p.vertices[2] - p.vertices[0];

    glm::vec3 pvec = glm::cross(r.direction, e2);
    float det = glm::dot(e1, pvec);

    if (glm::abs(det) < ERR) {
        return -1.0f;
    }

    float inv_det = 1.0f / det;
    glm::vec3 tvec = r.origin - p.vertices[0];
    float u = glm::dot(tvec, pvec) * inv_det;

    if (u < 0.0f || u > 1.0f) {
        return -1.0f;
    }

    glm::vec3 qvec = glm::cross(tvec, e1);
    float v = glm::dot(r.direction, qvec) * inv_det;

    if (v < 0.0f || u + v > 1.0f) {
        return -1.0f;
    }

    return glm::dot(e2, qvec) * inv_det;
}

bool visible(const glm::vec3 &a, const glm::vec3 &b, const patch &p_b,
             const std::vector<object> &world, float ERR) {

    ray r = {};
    r.origin = a;
    r.direction = glm::normalize(b - a);

    float t_other_b = intersect(r, p_b, ERR);

    for (const auto &o : world) {
        if (!intersect(r, o.box, ERR)) {
            continue;
        } else {
            for (const auto &p : o.patches) {
                float t = intersect(r, p, ERR);
                if (t > ERR && t < t_other_b) {
                    return false;
                }
            }
        }
    }

    return true;
}

float p2p_form_factor(const glm::vec3 &a, const glm::vec3 &n_a,
                      const glm::vec3 &b, const patch &p_b, float ERR, int FF_SAMPLES) {
    float r = glm::length(b - a);
    float denom = PI * r * r + p_b.area / FF_SAMPLES;
    if (denom < ERR) { return 0.0f; }

    glm::vec3 ab = glm::normalize(b - a);

    float cos_xy_na = glm::dot(ab, n_a);
    if (cos_xy_na <= 0.0f) { return 0.0f; }
    float cos_xy_nb = glm::dot(-1.0f * ab, p_b.normal);
    if (cos_xy_nb <= 0.0f) { return 0.0f; }

    float nom = glm::abs(cos_xy_na * cos_xy_nb); // normals are expected to be normalized!

    return nom / denom;
}

float form_factor(const patch &here, const patch &there, std::vector<object> &world,
                  float ERR, int FF_SAMPLES) {
    // from 'Radiosity and Realistic Image Synthesis' p. 95
    float F_ij = 0.0f;

    for (int k = 0; k < FF_SAMPLES; k++) {
        glm::vec3 here_p = sample_point(here);
        glm::vec3 there_p = sample_point(there);

        if (visible(here_p, there_p, there, world, ERR)) {
            float dF = p2p_form_factor(here_p, here.normal, there_p, there, ERR, FF_SAMPLES);
            if (dF > 0.0f) {
                F_ij += dF;
            }
        }
    }

    F_ij *= there.area; // there?

    return F_ij;
}

void iteration(std::vector<object> &objects, float ERR, int FF_SAMPLES) {
    for (auto &o : objects) {
        for (auto &p : o.patches) {
            glm::vec3 rad_new = glm::vec3(0.0f);

            float sum_ij = 0.0f;

            for (auto &o_other : objects) {
                for (auto &p_other: o_other.patches) {
                    float ff = form_factor(p, p_other, objects, ERR, FF_SAMPLES);

                    rad_new += p_other.rad * ff;
                    sum_ij += ff;
                }
            }

            rad_new *= p.color;
            rad_new += p.emit;

            p.rad_new = rad_new;
        }

    }

    for (auto &o : objects) {
        for (auto &p : o.patches) {
            p.rad = p.rad_new;
        }
    }

}

void reinhard(std::vector<object> &objects) {
    float N = 0;

    for (const auto &o : objects) {
        N += o.patches.size();
    }

    const glm::vec3 a = glm::vec3(0.5f); // TODO: wtf is mid-gray??
    glm::vec3 product = glm::vec3(1.0f);

    for (auto &o : objects) {
        for (auto &p : o.patches) {
            product *= glm::vec3(
                    p.rad.x == 0 ? 1.0f : std::pow(p.rad.x, 1.0f / N),
                    p.rad.y == 0 ? 1.0f : std::pow(p.rad.y, 1.0f / N),
                    p.rad.z == 0 ? 1.0f : std::pow(p.rad.z, 1.0f / N));
        }
    }

    glm::vec3 L_avg = product;

    for (auto &o : objects) {
        for (auto &p : o.patches) {
            p.rad = a / L_avg * p.rad;
        }
    }

    for (auto &o : objects) {
        for (auto &p : o.patches) {
            p.rad = p.rad / (glm::vec3(1.0f) + p.rad);
        }
    }

}
