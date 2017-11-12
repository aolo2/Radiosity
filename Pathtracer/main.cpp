#include "file_utils.h"
#include "camera.h"

using namespace utils;

int main() {
    std::vector<glm::vec3> film(static_cast<unsigned long>(
                                        constants::window_width * constants::window_height));

    camera cam(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f));

    for (unsigned int y = 0; y < constants::window_height; y++) {
        for (unsigned int x = 0; x < constants::window_width; x++) {
            glm::vec3 color;

            for (unsigned int ray_id = 0; ray_id < constants::spp; ray_id++) {
                ray r = cam.sample_pixel(x, y);
                color += glm::vec3(0.5f); // FIXME(aolo2): actually trace here!
            }

            color *= constants::inv_spp;
            m2d_write(film, x, y, color);
        }
    }

    utils::write_ppm(film);

    return 0;
}

/* TODO: intersection code, model loading

 */