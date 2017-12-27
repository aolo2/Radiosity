#include <iostream>
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

glm::vec3 sample_point(const patch *p) {

    float r1 = unilateral(mt);
    float r2 = unilateral(mt);

    return glm::vec3((1 - glm::sqrt(r1)) * p->vertices[0]
                     + glm::sqrt(r1) * (1 - r2) * p->vertices[1]
                     + r2 * glm::sqrt(r1) * p->vertices[2]);
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

bool visible(const glm::vec3 &a, const glm::vec3 &b, const patch *p_b,
             const bvh_node *world, const std::vector<patch *> &primitives, float ERR) {

    ray r = {};
    r.origin = a;
    r.direction = glm::normalize(b - a);

    float t_other_b = intersect(r, *p_b, ERR);
#ifndef LOCAL
    float t_world = intersect(r, world, primitives, ERR);
#endif

#if 0
    float t = INF;
    for (auto p : primitives) {
        float t_now = intersect(r, *p, ERR);
        if (t_now > ERR && t_now < t_other_b) {
            t = std::min(t, t_now);
        }
    }

//    if (t > ERR && t < t_other_b) {
//        std::cout << t_world - t << std::endl;
//    }
    return !(t < t_other_b && t > ERR);
#endif

#ifndef LOCAL
    return !(t_world > ERR && t_world < t_other_b);
#endif
    return false;
}

float p2p_form_factor(const glm::vec3 &a, const glm::vec3 &n_a,
                      const glm::vec3 &b, const patch *p_b, float ERR, int FF_SAMPLES) {
    float r = glm::length(b - a);
    float denom = PI * r * r + p_b->area / FF_SAMPLES;
    if (denom < ERR) { return 0.0f; }

    glm::vec3 ab = glm::normalize(b - a);

    float cos_xy_na = glm::dot(ab, n_a);
    if (cos_xy_na <= ERR) { return 0.0f; }
    float cos_xy_nb = glm::dot(-1.0f * ab, p_b->normal);
    if (cos_xy_nb <= ERR) { return 0.0f; }

    float nom = cos_xy_na * cos_xy_nb; // normals are expected to be normalized!

    return nom / denom;
}

float form_factor(const patch *here, const patch *there,
                  const bvh_node *world, const std::vector<patch *> &primitives,
                  float ERR, int FF_SAMPLES) {
    // from 'Radiosity and Realistic Image Synthesis' p. 95
    float F_ij = 0.0f;

    for (int k = 0; k < FF_SAMPLES; k++) {
        glm::vec3 here_p = sample_point(here);
        glm::vec3 there_p = sample_point(there);

        if (visible(here_p, there_p, there, world, primitives, ERR)) {
            float dF = p2p_form_factor(here_p, here->normal, there_p, there, ERR, FF_SAMPLES);
            if (dF > 0.0f) {
                F_ij += dF;
            }
        }
    }

    F_ij *= there->area; // there?

    return F_ij;
}

void iteration(const bvh_node *world, const std::vector<patch *> &primitives,
               float ERR, int FF_SAMPLES) {

    for (auto p : primitives) {
        glm::vec3 rad_new = glm::vec3(0.0f);

        for (auto p_other: primitives) {
            float ff = form_factor(p, p_other, world, primitives, ERR, FF_SAMPLES);
            rad_new += p_other->rad * ff;
        }

        rad_new *= p->color;
        rad_new += p->emit;

        p->rad_new = rad_new;
    }


    for (auto p : primitives) {
        p->rad = p->rad_new;
    }
}

void reinhard(std::vector<patch *> &primitives) {
    float N = primitives.size();

    const glm::vec3 a = glm::vec3(0.18f); // TODO: wtf is mid-gray??
    glm::vec3 product = glm::vec3(1.0f);

    for (auto &p : primitives) {
        product *= glm::vec3(
                p->rad.x == 0 ? 1.0f : std::pow(p->rad.x, 1.0f / N),
                p->rad.y == 0 ? 1.0f : std::pow(p->rad.y, 1.0f / N),
                p->rad.z == 0 ? 1.0f : std::pow(p->rad.z, 1.0f / N));
    }

    glm::vec3 L_avg = product;

    for (auto &p : primitives) {
        p->rad = a / L_avg * p->rad;
    }

    for (auto &p : primitives) {
        p->rad = p->rad / (glm::vec3(1.0f) + p->rad);
    }
}

#ifdef LOCAL

glm::vec3 sample_hemi(const glm::vec3 &normal) {
    glm::vec3 tan;
    if (glm::abs(glm::normalize(normal).y) > 0.99f) {
        tan = glm::vec3(1.0f, 0.0f, 0.0f);
    } else {
        tan = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), normal));
    }

    glm::vec3 bitan = glm::normalize(glm::cross(normal, tan));

    float u = unilateral(mt);
    float v = unilateral(mt);

    float cos_theta = glm::sqrt(1 - u);
    float sin_theta = glm::sqrt(1 - cos_theta * cos_theta);
    float phi = 2 * PI * v;

    float x = sin_theta * glm::cos(phi);
    float y = cos_theta;
    float z = sin_theta * glm::sin(phi);

    return glm::normalize(tan * x + normal * y + bitan * z);
}

void local_line(std::vector<patch *> &primitives, const long N, const bvh_node *world, float ERR) {
    /* Local-line stohastic incremental Jacobibi Radiosity (sec. 6.3 Advanced GI) */
    float total_unshot(0.0f);
    float last_unshot(0.0f);
    float total_power(0.0f);

    for (auto p : primitives) {
        p->p_total = p->emit.x * p->area;
        p->p_unshot = p->emit.x * p->area;
        p->p_recieved = 0.0f;
        total_unshot += p->p_unshot;
        total_power += p->p_total;
    }

    last_unshot = total_unshot;

    long N_prev;
    float q;

    while (total_unshot > 1e-10) {
//    for (int ii = 0; ii < 2; ii++) { // ONLY SECOND BOUNCE (total = received)
        auto N_samples = (long) (N * last_unshot / total_power);
        float xi = unilateral(mt);  // (0, 1)

//        std::cout << "Total unshot power = " << total_unshot << std::endl;
        std::cout << "N: " << N_samples << std::endl;

        N_prev = 0;
        q = 0;                // stratified imp. sampling

        for (auto p : primitives) {
            float q_i = p->p_unshot / total_unshot;
            q += q_i;
            long N_i = (long) glm::floor(N_samples * q + xi) - N_prev;

            for (long i = 0; i < N_i; ++i) {
                glm::vec3 x = sample_point(p);

                // TODO: precomute tangent and bitangent for each patch?
                ray sample = {x, sample_hemi(p->normal)};

                hit nearest = intersect(sample, world, primitives, ERR);
                if (nearest.hit && nearest.p != p) {
                    nearest.p->p_recieved += (1.0f / N_samples) * total_unshot; // * nearest.p->color.x
                }
            }

            N_prev += N_i;
        }

        last_unshot = total_unshot;
        total_power = 0.0f;
        total_unshot = 0.0f;

        for (auto p : primitives) {
            p->p_total += p->p_recieved;
            p->p_unshot = p->p_recieved;
            p->p_recieved = 0.0f;
            total_unshot += p->p_unshot;
            total_power += p->p_total;
        }
    }
    // display temp results
    //}
}

#endif