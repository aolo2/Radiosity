#include <glm/glm.hpp>
#include "radiance.h"
#include "constants.h"
#include "sampler.h"

glm::vec3 simple_rt(const point &here, const glm::vec3 &towards,
                    const utils::scene &scene, unsigned int depth) {

    if (depth > constants::max_bounces) {
        return glm::vec3(0.0f);
    }

    if (here.materialID == 0) {
        return scene.get_material(0).emit;
    }

    utils::material mat_here = scene.get_material(here.materialID);

    if (mat_here.emit != glm::vec3(0.0f)) {
        return mat_here.emit; // TODO(aolo2): emissive surfaces should reflect too, but arteeefaaacts
    }

    glm::vec3 estimated_radiance(0.0f);
    glm::vec3 next_ray;
    point next_hit;

    // TODO: more than one sample in the hemisphere?
    std::uniform_real_distribution<float> angle(0.0f, constants::pi);
    glm::vec3 tan_right = glm::cross(constants::world_up, here.normal);
    glm::vec3 tan_up = glm::cross(here.normal, tan_right);

    float phi = angle(utils::mt) * 2.0f;
    float theta = angle(utils::mt);

    float x = glm::sin(theta) * glm::cos(phi);
    float y = glm::sin(theta) * glm::sin(phi);
    float z = glm::cos(theta);

    glm::vec3 sample(x, y, z);

    if (here.normal != constants::world_up) {
        sample = sample.x * tan_right + sample.y * tan_up + sample.z * here.normal;
    }

    next_ray = glm::normalize(sample);
    next_hit = scene.trace({here.position, next_ray});

    estimated_radiance += simple_rt(next_hit, -1.0f * next_ray, scene, depth + 1);
    estimated_radiance *= mat_here.diffuse;

    return estimated_radiance;
}

glm::vec3 radiance(const ray &ray, const utils::scene &scene) {
    point hit = scene.trace(ray);
    return simple_rt(hit, -1.0f * ray.direction, scene, 0);
}
