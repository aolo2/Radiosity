#define TINYOBJLOADER_IMPLEMENTATION

#include <fstream>
#include <sstream>
#include <limits>
#include <glm/glm.hpp>
#include <iostream>

#include "scene.h"
#include "constants.h"
#include "tiny_obj_loader.h"


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

        for (const auto &object : objects) {
            for (const auto &triangle : object.mesh) {
                hit_t = intersect_triangle(r, triangle);

                if (hit_t > constants::error && hit_t < min_t) {
                    min_t = hit_t;
                    hit.materialID = object.materialID;
                    hit.normal = triangle.normal;
                }
            }
        }

        if (hit.materialID == 0) {
            return {glm::vec3(0.0f), glm::vec3(0.0f), 0};
        }

        hit.position = r.origin + r.direction * min_t;

//        if (glm::dot(hit.normal, -1.0f * r.direction) < 0.0f) {
//            hit.normal *= -1.0f;
//        }

        return hit;
    }

    void scene::add_objects(std::vector<object> &&objs) {
        objects.insert(objects.end(), objs.begin(), objs.end());
    }


    std::vector<object> load_mesh(const std::string &file_path) {
        tinyobj::attrib_t attrib;

        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::vector<object> objects(5); // FIXME: max materials count (or a map maybe?)
        for (unsigned int i = 0; i < objects.size(); i++) {
            objects[i].materialID = i;
        }

        std::string err;

        bool status = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, file_path.c_str());

        if (!err.empty()) {
            std::cerr << err << std::endl;
        }

        if (!status) {
            std::exit(1);
        }

        for (const auto &shape : shapes) {
            std::size_t index_offset = 0;

            /* Vertices */
            for (std::size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                int fv = shape.mesh.num_face_vertices[f];

                std::vector<glm::vec3> face_vertices;
                glm::vec3 normal;

                for (std::size_t v = 0; v < fv; v++) {
                    tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                    /* NOTE! We use mathematical XYZ for world coordinates, not OpenGL-ish */
                    float x = attrib.vertices[3 * idx.vertex_index + 0];
                    float z = attrib.vertices[3 * idx.vertex_index + 1]; // Z!
                    float y = attrib.vertices[3 * idx.vertex_index + 2]; // Y!

                    normal.x = attrib.normals[3 * idx.normal_index + 0];
                    normal.z = attrib.normals[3 * idx.normal_index + 1]; // Z!
                    normal.y = attrib.normals[3 * idx.normal_index + 2]; // Y!

                    face_vertices.emplace_back(x, y, z);
                }

                index_offset += fv;

                /* Materials */
                int current_material_id = shape.mesh.material_ids[f] + 1;

                if (current_material_id < 1) {
                    std::cerr << "Invalid material id" << std::endl;
                    std::exit(1);
                }

                objects[current_material_id].mesh
                        .push_back({face_vertices[0],
                                    face_vertices[1],
                                    face_vertices[2],
                                    normal});

            }
        }

        return objects;
    }
}