#include <iostream>
#include <iomanip>
#include <algorithm>

#include "../includes/radiosity.h"
#include "../includes/utils.h"

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_real_distribution<float> unilateral(0.0f, 1.0);

const float PI = 3.1415926f;
const std::string WAVES[] = {"RED", "GREEN", "BLUE"};

float area(const patch &p) {
    glm::vec3 a = p.vertices[0];
    glm::vec3 b = p.vertices[1];
    glm::vec3 c = p.vertices[2];

    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;

    float ab_len = glm::length(ab);
    float ac_len = glm::length(ac);

    float denom = ab_len * ac_len;
    if (denom < 1e-4) { return 0.0f; }

    float cos = glm::dot(ab, ac) / (ab_len * ac_len);
    if (glm::abs(cos - 1.0f) < 1e-4) { return 0.0f; }

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

void reinhard(std::vector<float> &vertices) {
    std::size_t vert_count = vertices.size() / 6; // 3 coords + 3 colors

    double log_space_sum[3] = {0.0, 0.0, 0.0};
    double MIN_COLOR = 1e-3;
    double inv_n = 1.0 / (double) vert_count;

    for (int wave_len = 0; wave_len < 3; ++wave_len) {
        for (std::size_t i = 0; i < vert_count; ++i) {
            if (vertices[i * 6 + 3 + wave_len] > MIN_COLOR) {
                auto old = log_space_sum[wave_len];
                log_space_sum[wave_len] += inv_n * glm::log(vertices[i * 6 + 3 + wave_len]);
                if (log_space_sum[wave_len] == INF) {
                    std::cout << old << " + " << vertices[i * 6 + 3 + wave_len] << std::endl;
                    break;
                }
            }
        }
    }

    double L_avg[3];
    for (int wave_len = 0; wave_len < 3; ++wave_len) {
        std::cout << log_space_sum[wave_len] << std::endl;
        L_avg[wave_len] = glm::exp(log_space_sum[wave_len]);
        std::cout << L_avg[wave_len] << std::endl;
    }

    double mid_gray(0.18); // 18% middle gray

    for (int wave_len = 0; wave_len < 3; ++wave_len) {
        for (std::size_t i = 0; i < vert_count; ++i) {
            vertices[i * 6 + 3 + wave_len] = (float) (mid_gray / L_avg[wave_len] * vertices[i * 6 + 3 + wave_len]);
            vertices[i * 6 + 3 + wave_len] = vertices[i * 6 + 3 + wave_len] / (1.0f + vertices[i * 6 + 3 + wave_len]);
        }
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

inline float sum(const glm::vec3 &v) {
    return v.x + v.y + v.z;
}

/* Transform per-patch constant radiosity to per-vertex values */
void interpolate(std::vector<patch *> &primitives, bvh_node *world, int G_RAYS, int S_RAYS, float ERR) {

    /* For each disc. wavelength */
    for (int wave_len = 0; wave_len < 3; wave_len++) {
        float nn = 1;

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

                p->colors[v][wave_len] = 0.0f;

                for (int i = 0; i < S_RAYS / 3; i++) {

                    patch *emitter = emitters[(int) std::round((unilateral(mt) * (emitters.size() - 1)))];
                    glm::vec3 Ep = sample_point(emitter);

                    if (visible(x, Ep, emitter, world, primitives, ERR)) {

                        glm::vec3 xy = Ep - p->vertices[v];
                        glm::vec3 xy_norm = glm::normalize(xy);

                        float r = glm::length(xy);
                        float G = 0.0f;

                        if (r > ERR) {
                            G = glm::dot(xy_norm, p->normal) * glm::dot(-xy_norm, emitter->normal) / (r * r);
                        }

                        p->colors[v][wave_len] += emitter->p_total[wave_len] * G;
                    }
                }


                // (N_L / N) * SUM_i^N   P_i * (color / PI) * G * V
                p->colors[v][wave_len] *= (p->color[wave_len] / PI * emitters.size() / (S_RAYS / 3)) * 500.0f;


                /* Indirect illumination */
                float B = 0.0f;

                for (int i = 0; i < G_RAYS / 3; i++) {
                    ray sample = {x, sample_hemi(p->normal)};
                    hit nearest = intersect(sample, world, primitives, ERR);

                    if (nearest.hit && nearest.p != p && nearest.p->emit[wave_len] < ERR) {
                        B += nearest.p->p_total[wave_len];
                    }
                }

                if (G_RAYS > 0) {
                    p->colors[v][wave_len] += p->color[wave_len] * B / G_RAYS * 3;
                }
            }


            auto percent_done = (int) (100.0f * (nn++ / primitives.size()));

            std::cout << "\rInterpolating[" << WAVES[wave_len] << "]... ";
            if (percent_done >= 10) {
                std::cout << percent_done;
            } else {
                std::cout << "0" << percent_done;
            }

            std::cout << "%" << std::flush;
        }

        std::cout << std::endl;
    }

    /*  std::cout << "Removing artefacts... " << std::flush;
      for (auto p : primitives) {
          glm::vec3 max_color(0.0f);

          for (int v = 0; v < 3; v++) {
              if (sum(p->colors[v]) > sum(max_color)) {
                  max_color = p->colors[v];
              }
          }

          for (int v = 0; v < 3; v++) {
              if (sum(p->colors[v]) < 0.02f && p->percent_visible < 0.1f) {
                  p->colors[v] = max_color;
              }
          }
      }
      std::cout << "DONE" << std::endl;*/
}

/* Local-line stohastic incremental Jacobi Radiosity (sec. 6.3 Advanced GI) */
void local_line(std::vector<patch *> &primitives, const settings &s, const bvh_node *world, stats &stat) {

    double sijia_start = glfwGetTime();
    stat.events[EVENT::SIJIA_BEGIN] = glfwGetTime();

    for (auto p : primitives) {
        p->p_total = p->emit * p->area;
        p->p_unshot = p->emit * p->area;
        p->p_recieved = glm::vec3(0.0f);
    }

    int iteration_count = 0;

/*    GLuint dbg_VAO, dbg_VBO;
    std::vector<float> ray_info = {
            0.0f, 0.0f, 0.0f, // start pos
            0.0f, 0.0f, 0.0f, // start color

            0.0f, 0.0f, 0.0f, // end pos
            0.0f, 0.0f, 0.0f  // end color
    };

    if (s.debug) {
        init_buffers(&dbg_VAO, &dbg_VBO, ray_info);
        glBindVertexArray(dbg_VAO);

        if (s.debug) {
            glm::vec3 ray_color(1.0f, 0.0f, 0.0f);
            if (nearest.hit) { ray_color = glm::vec3(0.0f, 1.0f, 0.0f); }

            ray_info = {x[0], x[1], x[2], ray_color[0], ray_color[1], ray_color[2]};

            if (nearest.hit) {
                ray_info.push_back(nearest.p->vertices[4].x);
                ray_info.push_back(nearest.p->vertices[4].y);
                ray_info.push_back(nearest.p->vertices[4].z);
            } else {
                glm::vec3 far_miss_point = sample.direction + sample.direction * 10000.0f;
                ray_info.push_back(far_miss_point.x);
                ray_info.push_back(far_miss_point.y);
                ray_info.push_back(far_miss_point.z);
            }

            ray_info.push_back(ray_color[0]);
            ray_info.push_back(ray_color[1]);
            ray_info.push_back(ray_color[2]);

            update_buffers(&dbg_VAO, &dbg_VBO, ray_info);
            glDrawArrays(GL_LINES, 0, 2);
        }
    }*/

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
        long long N_prev;
        float q;

        /* Incremental shooting */
        while (total_unshot > 1e-7) {
            auto N_samples = (long long) (s.TOTAL_RAYS * total_unshot / total_power);
            float xi = unilateral(mt);

            if (s.verbose) {
                std::cout << "Unshot "
                          << WAVES[wave_len] << ": " << std::left << std::setw(15)
                          << total_unshot << "\r" << std::flush;
            }

            N_prev = 0;
            q = 0;

            for (auto p : primitives) {
                if (N_prev == N_samples) { break; }
                auto q_i = p->p_unshot[wave_len] / total_unshot;
                q += q_i;
                long N_i = (long) glm::floor(N_samples * q + xi) - N_prev;

                for (long i = 0; i < N_i; ++i) {
                    glm::vec3 x = sample_point(p);
                    ray sample = {x, sample_hemi(
                            p->normal)}; // TODO: precompute tangent and bi-tangent for each patch?
                    hit nearest = intersect(sample, world, primitives, s.ERR);

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

            ++iteration_count;
        }

        if (s.verbose) { std::cout << std::endl; }
    }

    for (auto p : primitives) {
        for (int wave_len = 0; wave_len < 3; wave_len++) {
            if (p->area < 1e-2) {
                p->colors[0][wave_len] =
                p->colors[1][wave_len] =
                p->colors[2][wave_len] = p->p_total[wave_len];
            } else {
                p->colors[0][wave_len] =
                p->colors[1][wave_len] =
                p->colors[2][wave_len] = p->p_total[wave_len] / p->area;
            }
        }
    }

    double sijia_end = glfwGetTime();

    stat.events[EVENT::SIJIA_END] = glfwGetTime();
    stat.iterations_number = iteration_count;
}
