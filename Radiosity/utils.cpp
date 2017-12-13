#define TINYOBJLOADER_IMPLEMENTATION

#include "tiny_obj_loader.h"
#include "utils.h"
#include "radiosity.h"

#include <dirent.h>
#include <iostream>

settings load_settings(const std::string &path) {
    std::ifstream file(path.c_str());

    settings s = {};

    file >> s.WINDOW_WIDTH
         >> s.WINDOW_HEIGHT
         >> s.ERR
         >> s.FOV
         >> s.RAD_ITERATIONS
         >> s.FF_SAMPLES
         >> s.camera_pos.x
         >> s.camera_pos.y
         >> s.camera_pos.z
         >> s.mesh_path;

    s.ASPECT_RATIO =
            static_cast<float>(s.WINDOW_WIDTH) /
            static_cast<float>(s.WINDOW_HEIGHT);

    return s;
}

std::vector<patch> load_mesh(const std::string &path) {
    std::vector<patch> patches;
    tinyobj::attrib_t attrib;

    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;

    bool status = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str());

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!status) {
        std::exit(1);
    }

    for (auto &shape : shapes) {
        std::size_t index_offset = 0;

        std::cout << "Loading object \'" << shape.name << "\'" << std::endl;

        /* Vertices */
        for (std::size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            patch p = {};

            if (fv != 3) {
                std::cerr << "Scene is not triangulated!" << std::endl;
                std::exit(1);
            }

            for (std::size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                p.vertices[v] = glm::vec3(
                        attrib.vertices[3 * idx.vertex_index + 0],
                        attrib.vertices[3 * idx.vertex_index + 1],
                        attrib.vertices[3 * idx.vertex_index + 2]);

                p.normal = glm::vec3(
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]);
            }

            p.vertices[3] = (p.vertices[0] + p.vertices[1] + p.vertices[2]) / 3.0f; // centroid
            p.normal = glm::normalize(p.normal);

            /* Materials */
            int current_material_id = shape.mesh.material_ids[f];

            if (current_material_id < 0) {
                std::cerr << "Material not specified" << std::endl;
                continue;
            }


            p.color = glm::vec3(
                    materials[current_material_id].diffuse[0],
                    materials[current_material_id].diffuse[1],
                    materials[current_material_id].diffuse[2]);

            p.area = area(p);

            p.emit = glm::vec3(
                    materials[current_material_id].ambient[0],
                    materials[current_material_id].ambient[1],
                    materials[current_material_id].ambient[2]);

            patches.push_back(p);

            index_offset += fv;
        }
    }

    return patches;
}

std::vector<float> glify(const std::vector<patch> &patches) {
    std::vector<float> vertices;

    for (const auto &p : patches) {
        vertices.push_back(p.vertices[0].x);
        vertices.push_back(p.vertices[0].y);
        vertices.push_back(p.vertices[0].z);

        vertices.push_back(p.rad.x);
        vertices.push_back(p.rad.y);
        vertices.push_back(p.rad.z);

        vertices.push_back(p.vertices[1].x);
        vertices.push_back(p.vertices[1].y);
        vertices.push_back(p.vertices[1].z);

        vertices.push_back(p.rad.x);
        vertices.push_back(p.rad.y);
        vertices.push_back(p.rad.z);

        vertices.push_back(p.vertices[2].x);
        vertices.push_back(p.vertices[2].y);
        vertices.push_back(p.vertices[2].z);

        vertices.push_back(p.rad.x);
        vertices.push_back(p.rad.y);
        vertices.push_back(p.rad.z);
    }

    return vertices;
}

void init_buffers(GLuint *VAO, GLuint *VBO, const std::vector<float> &vertices) {
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);

    glBindVertexArray(*VAO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *) 0);
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *) (3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1); // color

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}