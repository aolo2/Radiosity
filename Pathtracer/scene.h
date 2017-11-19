#ifndef PATHTRACER_SCENE_H
#define PATHTRACER_SCENE_H


#include <string>
#include <vector>
#include "ray.h"

namespace utils {
    struct object {
        std::vector<triangle> mesh;
        unsigned int materialID;
    };

    struct material {
        glm::vec3 emit;
        glm::vec3 diffuse;
    };

    std::vector<object> load_mesh(const std::string &file_path);

    class scene {
    public:
        explicit scene(const std::string &materials_path);

        scene(const scene &other) = default;

        point trace(ray r) const;

        material get_material(unsigned int i) const {
            return i < materials.size() ?
                   materials[i] : materials[0]; // fallback to background
        }

        void add_object(const object &obj) {
            objects.push_back(obj);
        }

        void add_objects(std::vector<object> &&objs);

        void add_material(const material &mat) {
            materials.push_back(mat);
        }

        scene &operator=(scene other) = delete;

        scene &operator=(scene &&other) = delete;

        scene(scene &&other) = delete;

        ~scene() = default;

    private:
        // TODO(aolo2): kd-tree!!!
        std::vector<object> objects;
        std::vector<material> materials;
    };

}


#endif //PATHTRACER_SCENE_H
