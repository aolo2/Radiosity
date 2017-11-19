#include "file_utils.h"
#include "camera.h"
#include "radiance.h"

#include <iostream>

using namespace utils;

int main() {
    std::vector<glm::vec3> film(static_cast<unsigned long>(
                                        constants::window_width * constants::window_height));

    camera cam(glm::vec3(278.0f, -800.0f, 273.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    scene scn("resources/materials.txt");

    scn.add_objects(load_mesh("resources/untitled.obj"));

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

        std::cout << (float) y / constants::window_height * 100.0f << "%" << std::endl;
    }

    utils::write_ppm(film);

    return 0;
}