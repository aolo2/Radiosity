#ifndef PATHTRACER_CAMERA_H
#define PATHTRACER_CAMERA_H

#include <glm/vec3.hpp>
#include <random>
#include "ray.h"

namespace utils {
    class camera {
    public:
        camera(const glm::vec3 &position, const glm::vec3 &target);

        ray sample_pixel(unsigned int x, unsigned int y);

        camera(camera &&other) = delete;

        camera &operator=(camera &&other) = delete;

        camera &operator=(camera other) = delete;

        camera(const camera &other) = delete;

        ~camera() = default;

    private:
        /* Camera coordinate system */
        glm::vec3 position;     // Camera position (world coordinates)

        glm::vec3 x_axis;       // camera 'right'
        glm::vec3 y_axis;       // camera 'up'
        glm::vec3 z_axis;       // camera is directed towards negative 'z'

        /* Virtual 'screen' */
        glm::vec3 film_center;  // center of the screen (in world coordinates)
        float film_dist;        // distance from camera to the screen
        float film_width;       // screen width in world 'size'
        float film_height;

        float half_film_width;  // is needed for sampling
        float half_film_height;

        std::uniform_real_distribution<float> pixel_jitter;
    };
}

#endif //PATHTRACER_CAMERA_H
