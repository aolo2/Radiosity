#include <iostream>
#include <iomanip>
#include <algorithm>
#include "../includes/radiosity.h"
#include "../includes/utils.h"

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
    hit t_world = intersect(r, world, primitives, ERR);

    return !(t_world.hit && t_world.t > ERR && t_world.t < t_other_b);
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

glm::vec3 sample_hemi(const glm::vec3 &normal) {
    glm::vec3 tan;
    if (glm::abs(glm::normalize(normal).y) > 0.999f) {
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

/* Transform per-patch constant radiosity to per-vertex values */
void interpolate(std::vector<patch *> &primitives, bvh_node *world, int G_RAYS, int S_RAYS, float ERR) {

    /* For each disc. wavelength */
    for (int wave_len = 0; wave_len < 3; wave_len++) {
        std::cout << "." << std::flush;
        for (auto p : primitives) {

            if (p->emit[wave_len] > ERR) {
                p->colors[0][wave_len] = p->colors[1][wave_len] = p->colors[2][wave_len] = p->p_total[wave_len];
                continue;
            }

            /* For each vertex in the scene*/
            for (int v = 0; v < 3; v++) {

                glm::vec3 x = p->vertices[v];

                /* Separate light sources */
                float P_total = 0.0f;
                std::vector<patch *> emitters;
                for (auto prim : primitives) {
                    if (prim->emit[wave_len] > ERR) {
                        emitters.push_back(prim);
                        P_total += prim->p_total[wave_len];
                    }
                }

                /* Direct illumination */
                if (emitters.empty()) {
                    continue;
                }

                for (int i = 0; i < S_RAYS / 3; i++) {

                    patch *emitter = emitters[(int) std::round((unilateral(mt) * (emitters.size() - 1)))];
                    glm::vec3 Ep = sample_point(emitter);

                    if (visible(x, Ep, emitter, world, primitives, ERR)) {

                        glm::vec3 xy = Ep - p->vertices[v];
                        glm::vec3 xy_norm = glm::normalize(xy);

                        float r = glm::length(xy);
                        float G = glm::dot(xy_norm, p->normal) * glm::dot(-xy_norm, emitter->normal) / (r * r);

                        p->colors[v][wave_len] += emitter->p_total[wave_len] * G;
                    }
                }

                // (N_L / N) * SUM_i^N   P_i * (color / PI) * G * V
                p->colors[v][wave_len] *= (p->color[wave_len] / PI * emitters.size() / (S_RAYS / 3)) * 800.0f;


                /* Indirect illumination */
                float B = 0.0f;

                for (int i = 0; i < G_RAYS / 3; i++) {
                    ray sample = {x, sample_hemi(p->normal)};
                    hit nearest = intersect(sample, world, primitives, ERR);

                    if (nearest.hit && nearest.p != p && nearest.p->emit[wave_len] < ERR) {
                        B += nearest.p->p_total[wave_len];
                    }
                }

                p->colors[v][wave_len] += p->color[wave_len] * B / G_RAYS * 3;
            }
        }
    }
}

/* Local-line stohastic incremental Jacobi Radiosity (sec. 6.3 Advanced GI) */
void local_line(std::vector<patch *> &primitives, const long N, const bvh_node *world, float ERR) {

    float total_area = 0.0f;

    for (auto p : primitives) {
        p->p_total = p->emit * p->area;
        p->p_unshot = p->emit * p->area;
        p->p_recieved = glm::vec3(0.0f);
        total_area += p->area;
    }

    /* Run the simulation for each wavelength */
    for (int wave_len = 0; wave_len < 3; ++wave_len) {

        /* Init total powers to zero */
        float total_unshot(0.0f);
        float last_unshot(0.0f);
        float total_power(0.0f);

        /* Init power for fixed wavelength */
        for (auto p : primitives) {
            total_unshot += p->p_unshot[wave_len];
            total_power += p->p_total[wave_len];
        }

        /* Stratified sampling */
        long N_prev;
        float q;

        /* Incremental shooting */
        while (total_unshot > 1e-7) {
            auto N_samples = (long) (N * total_unshot / total_power);
            float xi = unilateral(mt);

            std::cout << "Total unshot power: " << total_unshot << "\r" << std::flush;

            N_prev = 0;
            q = 0;

            for (auto p : primitives) {
                q += p->p_unshot[wave_len] / total_unshot;
                long N_i = (long) glm::floor(N_samples * q + xi) - N_prev;

                for (long i = 0; i < N_i; ++i) {
                    glm::vec3 x = sample_point(p);
                    ray sample = {x, sample_hemi(
                            p->normal)}; // TODO: precompute tangent and bi-tangent for each patch?
                    hit nearest = intersect(sample, world, primitives, ERR);

                    if (nearest.hit && nearest.p != p) {
                        nearest.p->p_recieved[wave_len] +=
                                (1.0f / N_samples) * total_unshot * nearest.p->color[wave_len];
                    }
                }

                N_prev += N_i;
            }

            total_power = 0.0f;
            total_unshot = 0.0f;

            for (auto p : primitives) {
                p->p_total[wave_len] += p->p_recieved[wave_len];
                p->p_unshot[wave_len] = p->p_recieved[wave_len];
                p->p_recieved = glm::vec3(0.0f);
                total_unshot += p->p_unshot[wave_len];
                total_power += p->p_total[wave_len];
            }
        }

        std::cout << "\nDone." << std::endl;
    }
}