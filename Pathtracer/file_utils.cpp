#include "constants.h"
#include "file_utils.h"

namespace utils {
    void m2d_write(std::vector<glm::vec3> &data, uint x, uint y, glm::vec3 val) {
        data[(constants::window_height - y - 1) * constants::window_width + x] = val;
    }

    glm::vec3 m2d_read(const std::vector<glm::vec3> &data, uint x, uint y) {
        return data[y * constants::window_width + x];
    }

    inline float srgb(float linear) {
        return glm::pow(linear, 1.0f / 2.2f);
    }

    void write_ppm(const std::vector<glm::vec3> &film) {
        std::ofstream image("image.ppm");
        image << "P3 " << constants::window_width << " " << constants::window_height
              << " " << constants::max_colors << std::endl;

        for (uint y = 0; y < constants::window_height; y++) {
            for (uint x = 0; x < constants::window_width; x++) {

                glm::vec3 pixel = glm::clamp(m2d_read(film, x, y), 0.0f, 1.0f);

                image << (unsigned int) (srgb(pixel.x) * constants::max_colors) << " "
                      << (unsigned int) (srgb(pixel.y) * constants::max_colors) << " "
                      << (unsigned int) (srgb(pixel.z) * constants::max_colors) << " ";
            }

            image << std::endl;
        }
    }
}