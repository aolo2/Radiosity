#define TINYOBJLOADER_IMPLEMENTATION

#include "../includes/tiny_obj_loader.h"
#include "../includes/utils.h"
#include "../includes/radiosity.h"

#include <dirent.h>
#include <iostream>

void load_settings(const std::string &path, settings &s) {
    std::ifstream file(path.c_str());

    file >> s.WINDOW_WIDTH
         >> s.WINDOW_HEIGHT
         >> s.ERR
         >> s.FOV
         >> s.camera_pos.x
         >> s.camera_pos.y
         >> s.camera_pos.z
         >> s.mesh_path
         >> s.TOTAL_RAYS;

    s.ASPECT_RATIO =
            static_cast<float>(s.WINDOW_WIDTH) /
            static_cast<float>(s.WINDOW_HEIGHT);
}

std::vector<patch> load_mesh(const std::string &path, stats &stat) {
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

//        std::cout << "Loading object \'" << shape.name << "\'" << std::endl;

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
                std::exit(1);
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

            ++stat.polygons_count;

            if (p.emit != glm::vec3(0.0f)) {
                ++stat.light_sources_count;
            }

            patches.push_back(p);

            index_offset += fv;
        }
    }

    return patches;
}

std::vector<float>  glify(const std::vector<patch *> &primitives, bool fill) {
    std::vector<float> vertices;

    for (const auto p : primitives) {
        vertices.push_back(p->vertices[0].x);
        vertices.push_back(p->vertices[0].y);
        vertices.push_back(p->vertices[0].z);

        vertices.push_back(fill ? 0.6f : p->colors[0].r);
        vertices.push_back(fill ? 0.6f : p->colors[0].g);
        vertices.push_back(fill ? 0.6f : p->colors[0].b);

        vertices.push_back(p->vertices[1].x);
        vertices.push_back(p->vertices[1].y);
        vertices.push_back(p->vertices[1].z);

        vertices.push_back(fill ? 0.6f : p->colors[1].r);
        vertices.push_back(fill ? 0.6f : p->colors[1].g);
        vertices.push_back(fill ? 0.6f : p->colors[1].b);

        vertices.push_back(p->vertices[2].x);
        vertices.push_back(p->vertices[2].y);
        vertices.push_back(p->vertices[2].z);

        vertices.push_back(fill ? 0.6f : p->colors[2].r);
        vertices.push_back(fill ? 0.6f : p->colors[2].g);
        vertices.push_back(fill ? 0.6f : p->colors[2].b);
    }

    return vertices;
}

void update_buffers(GLuint *VAO, GLuint *VBO, std::vector<float> &vertices) {
    glBindVertexArray(*VAO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);

    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(GLfloat), vertices.data());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *) 0);
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *) (3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1); // color

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void init_buffers(GLuint *VAO, GLuint *VBO, std::vector<float> &vertices) {
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);

    glBindVertexArray(*VAO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *) 0);
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *) (3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1); // color

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

settings process_flags(int argc, char **argv) {
    settings s = {};

    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (ALLOWED_FLAGS.find(arg) == ALLOWED_FLAGS.end()) {
            std::cerr << "Unknown parameter: \'" << argv[i] << "\'" << std::endl;
            s.invalid = true;
            return s;
        } else {
            if (arg == "-l") {
                s.display_only = true;
            } else if (arg == "-s") {
                s.save_result = true;
            } else if (arg == "-stats") {
                s.show_stats = true;
            } else if (arg == "-v") {
                s.verbose = true;
            } else if (arg == "-d") {
                s.debug = true;
            }
        }
    }

    if (s.display_only && s.save_result) {
        std::cout << "Both -l and -s flags set. Ignoring -s" << std::endl;
        s.save_result = false;
    }

    if (s.verbose) {
        std::cout << "Set flags: VERBOSE(-v) ";
        if (s.display_only) { std::cout << "DISPLAY ONLY(-l) " << std::flush; }
        if (s.save_result) { std::cout << "SAVE RESULT(-s) " << std::flush; }
        if (s.show_stats) { std::cout << "SHOW STATS(-stats) " << std::flush; }
        if (s.debug) { std::cout << "DEBUG MODE(-d) " << std::flush; }
        std::cout << std::endl;
    }

    return s;
}
