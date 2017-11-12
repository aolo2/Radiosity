#include "file_utils.h"
#include "camera.h"
#include "radiance.h"

using namespace utils;

int main() {
    std::vector<glm::vec3> film(static_cast<unsigned long>(
                                        constants::window_width * constants::window_height));

    camera cam(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f));
    scene scn("resources/materials.txt");

    scn.add_object({{{glm::vec3(-1.0f, 0.0f, 0.0f),
                      glm::vec3(0.0f, 0.0f, 1.0f),
                      glm::vec3(1.0f, 0.0f, 0.0f)}},
                    1});

    for (unsigned int y = 0; y < constants::window_height; y++) {
        for (unsigned int x = 0; x < constants::window_width; x++) {
            glm::vec3 color;

            for (unsigned int ray_id = 0; ray_id < constants::spp; ray_id++) {
                ray r = cam.sample_pixel(x, y);
                color += radiance(r, scn);
            }

            color *= constants::inv_spp;
            m2d_write(film, x, y, color);
        }
    }

    utils::write_ppm(film);

    return 0;
}