#include "camera.h"
#include "constants.h"
#include "sampler.h"

#include <glm/glm.hpp>

namespace utils {
    camera::camera(const glm::vec3 &position, const glm::vec3 &target)
            : position(position), pixel_jitter(0.0f, 1.0f) {

        z_axis = glm::normalize(position - target);
        x_axis = glm::normalize(glm::cross(constants::world_up, z_axis));
        y_axis = glm::normalize(glm::cross(z_axis, x_axis));

        film_dist = 1.0f;
        film_width = 1.0f;
        film_height = 1.0f;

        if (constants::window_width > constants::window_height) {
            film_height = constants::window_height / constants::window_width;
        } else {
            film_width = constants::window_width / constants::window_height;
        }

        film_center = position - film_dist * z_axis;

        half_film_width = film_width * 0.5f;
        half_film_height = film_height * 0.5f;
    }

    ray camera::sample_pixel(unsigned int x, unsigned int y) {
        glm::vec3 pixel_position;

        float x_jitter = pixel_jitter(utils::mt);
        float y_jitter = pixel_jitter(utils::mt);

        float pixel_x = (static_cast<float>(x) + x_jitter / constants::window_width) * 2.0f - 1.0f;
        float pixel_y = (static_cast<float>(y) + y_jitter / constants::window_height) * 2.0f - 1.0f;

        pixel_position = film_center
                         + pixel_x * x_axis * half_film_width
                         + pixel_y * y_axis * half_film_height;

        glm::vec3 ray_origin = position;
        glm::vec3 ray_direction = glm::normalize(pixel_position - position);

        return {ray_origin, ray_direction};
    }
}
