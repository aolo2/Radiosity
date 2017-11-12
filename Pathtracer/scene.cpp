#include <fstream>
#include <sstream>
#include <limits>
#include <glm/glm.hpp>
#include "scene.h"
#include "constants.h"

namespace utils {
    scene::scene(const std::string &materials_path) {
        std::fstream file(materials_path);
        std::string line;

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            glm::vec3 emit, diffuse;

            ss >> emit.x >> emit.y >> emit.z
               >> diffuse.x >> diffuse.y >> diffuse.z;

            materials.push_back({emit, diffuse});
        }
    }

    point scene::trace(ray r) const {
        float min_t = std::numeric_limits<float>::max();
        float hit_t;

        point hit = {};
        hit.materialID = 0;
        triangle closest_triangle;

        for (const auto &object : objects) {
            for (const auto &triangle : object.mesh) {
                hit_t = intersect_triangle(r, triangle);

                if (hit_t > constants::error && hit_t < min_t) {
                    min_t = hit_t;
                    hit.materialID = object.materialID;
                    closest_triangle = triangle;
                }
            }
        }

        if (hit.materialID == 0) {
            return {glm::vec3(0.0f), glm::vec3(0.0f), 0};
        }

        hit.position = r.origin + r.direction * min_t;
        hit.normal = glm::normalize(glm::cross(
                closest_triangle.c - closest_triangle.b,
                closest_triangle.a - closest_triangle.b)); // FIXME(aolo2): IMPORTANT!! ORIENTATION!!

        return hit;
    }
}