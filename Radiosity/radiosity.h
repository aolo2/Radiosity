#ifndef RADIOSITY_RADIOSITY_H
#define RADIOSITY_RADIOSITY_H

#include <random>
#include "shared.h"

float area(const patch &p);

glm::vec3 sample_point(const patch &p);

float intersect(const ray &r, const patch &p, float ERR);

bool visible(const glm::vec3 &a, const glm::vec3 &b,
             const patch &p_b, const std::vector<patch> &scene, float ERR);

float p2p_form_factor(const glm::vec3 &a, const glm::vec3 &n_a,
                      const glm::vec3 &b, const patch &p_b, float ERR, int FF_SAMPLES);

float form_factor(const patch &here,
                  const patch &there, std::vector<patch> &world,
                  float ERR, int FF_SAMPLES);

void iteration(std::vector<patch> &patches, float ERR, int FF_SAMPLES);

void reinhard(std::vector<patch> &patches);

#endif //RADIOSITY_RADIOSITY_H
