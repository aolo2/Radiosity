#ifndef PATHTRACER_RADIANCE_H
#define PATHTRACER_RADIANCE_H

#include <glm/vec3.hpp>
#include "ray.h"
#include "scene.h"

glm::vec3 radiance(const ray &ray, const utils::scene &scene);

glm::vec3 simple_rt(const point &here, const glm::vec3 &towards,
                    const utils::scene &scene, unsigned int depth);

#endif //PATHTRACER_RADIANCE_H
