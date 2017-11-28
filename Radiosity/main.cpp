#define TINYOBJLOADER_IMPLEMENTATION

#include "tiny_obj_loader.h"
#include "shader.h"

#include <GLFW/glfw3.h>
#include <random>

const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 1024;
const float ERR = 1e-5;
const float PI = 3.1415926f;
const float FOV = 0.4366f * 2.0f / PI * 180.0f;
const float ASPECT_RATIO = static_cast<float>(WINDOW_WIDTH) /
                           static_cast<float>(WINDOW_HEIGHT);

const int FF_SAMPLES = 16;
const int RAD_ITERATIONS = 16;

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_real_distribution<float> unilateral(0.0f, 1.0);

struct patch {
    glm::vec3 vertices[3];
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec3 rad;
    glm::vec3 rad_new;
    glm::vec3 emit;
    float area;
};

bool operator==(const patch &lhs, const patch &rhs) {
    return (glm::length(lhs.vertices[0] - rhs.vertices[0]) < ERR
            && glm::length(lhs.vertices[1] - rhs.vertices[1]) < ERR
            && glm::length(lhs.vertices[2] - rhs.vertices[2]) < ERR);
}

bool operator!=(const patch &lhs, const patch &rhs) {
    return !(lhs == rhs);
}

struct ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

float area(const patch &p) {
    glm::vec3 a = p.vertices[0];
    glm::vec3 b = p.vertices[1];
    glm::vec3 c = p.vertices[2];

    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;

    float ab_len = glm::length(ab);
    float ac_len = glm::length(ac);
    float cos = glm::dot(ab, ac) / (ab_len * ac_len);

    return 0.5f * ab_len * ac_len * glm::sqrt(1 - cos * cos);
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

//            p.rad = p.color;

            patches.push_back(p);

            index_offset += fv;
        }
    }

    return patches;
}

glm::vec3 sample_point(const patch &p) {

    float r1 = unilateral(mt);
    float r2 = unilateral(mt);

    return glm::vec3((1 - glm::sqrt(r1)) * p.vertices[0]
                     + glm::sqrt(r1) * (1 - r2) * p.vertices[1]
                     + r2 * glm::sqrt(r1) * p.vertices[2]);
}

//float ni_1 = 0, ni_2 = 0, ni_3 = 0;
//float pi = 0;

float interect(const ray &r, const patch &p) {
    glm::vec3 e1 = p.vertices[1] - p.vertices[0];
    glm::vec3 e2 = p.vertices[2] - p.vertices[0];

    glm::vec3 pvec = glm::cross(r.direction, e2);
    float det = glm::dot(e1, pvec);

    if (glm::abs(det) < ERR) {
//        ni_1++;
        return -1.0f;
    }

    float inv_det = 1.0f / det;
    glm::vec3 tvec = r.origin - p.vertices[0];
    float u = glm::dot(tvec, pvec) * inv_det;

    if (u < 0.0f || u > 1.0f) {
//        ni_2++;
        return -1.0f;
    }

    glm::vec3 qvec = glm::cross(tvec, e1);
    float v = glm::dot(r.direction, qvec) * inv_det;

    if (v < 0.0f || u + v > 1.0f) {
//        ni_3++;
        return -1.0f;
    }

    return glm::dot(e2, qvec) * inv_det;
}

float vis = 0, invis = 0;

bool visible(const glm::vec3 &a, const glm::vec3 &b, const patch &p_b, const std::vector<patch> &scene) {
    ray r = {};
    r.origin = a;
    r.direction = glm::normalize(b - a);

//    if ((glm::abs(a.x - b.x < ERR) && glm::abs(a.y - b.y < ERR))
//        || (glm::abs(a.x - b.x < ERR) && glm::abs(a.z - b.z < ERR))
//        || (glm::abs(a.y - b.y < ERR) && glm::abs(a.z - b.z < ERR))) {
//        invis++;
//        return false;
//    }

    float t_other_b = interect(r, p_b);

    for (const auto &p : scene) {
        float t = interect(r, p);
        if (t > ERR && t < t_other_b) {
            invis++;


//                std::cout << "Ray: " << std::endl
//                          << "  origin: (" << a.x << ", " << a.y << ", " << a.z << ")" << std::endl
//                          << "  direction: (" << r.direction.x << ", " << r.direction.y << ", " << r.direction.z << ")"
//                          << std::endl;
//                std::cout << "Patch: " << std::endl
//                          << "  A: (" << p.vertices[0].x << ", " << p.vertices[0].y << ", " << p.vertices[0].z << ")"
//                          << std::endl
//                          << "  B: (" << p.vertices[1].x << ", " << p.vertices[1].y << ", " << p.vertices[1].z << ")"
//                          << std::endl
//                          << "  C: (" << p.vertices[2].x << ", " << p.vertices[2].y << ", " << p.vertices[2].z << ")"
//                          << std::endl;
//
//
//                std::cout << std::endl;
            return false;
        }
    }
//FIXME: returns false almost always - investigate

    vis++;
    return true;
}

float p2p_form_factor(const glm::vec3 &a, const glm::vec3 &n_a, const glm::vec3 &b, const patch &p_b,
                      std::vector<patch> &world) {

    float denom = PI * glm::length(b - a) * glm::length(b - a);
    if (denom < ERR) { return 0.0f; }

    if (!visible(a, b, p_b, world)) {
        return 0.0f;
    }


    glm::vec3 ab = glm::normalize(b - a);
    float cos_xy_na = glm::dot(ab, n_a);
//    if (cos_xy_na < 0.0f) { return 0.0f; }
    float cos_xy_nb = glm::dot(-1.0f * ab, p_b.normal);
//    if (cos_xy_nb < 0.0f) { return 0.0f; }

    float nom = cos_xy_na * cos_xy_nb; // normals are expected to be normalized!

    return nom / denom;
}

float form_factor(const patch &here, const patch &there, std::vector<patch> &world) {
    float F_ij = 0.0f;

    float here_prob = here.area / (here.area + there.area);

//    if (here_prob - 0.5f < ERR) {
//        if (here == there) {
//            return 0.0f;
//        }
//    }

    for (int i = 0; i < FF_SAMPLES; i++) {
        glm::vec3 point_here = sample_point(here);
        glm::vec3 point_there = sample_point(there);

        float p2p_ff = p2p_form_factor(point_here, here.normal, point_there, there, world);

        if (p2p_ff <= 0.0f) {
            continue;
        }

        float where = unilateral(mt);

        if (where > here_prob) {
            p2p_ff *= there.area;
        } else {
            p2p_ff *= here.area;
        }

        F_ij += p2p_ff;
    }

    F_ij /= static_cast<float>(FF_SAMPLES);

    return F_ij;
}

void iteration(std::vector<patch> &patches) {
    for (auto &p : patches) {
        glm::vec3 rad_new = glm::vec3(0.0f);

        for (auto &p_other: patches) {
            rad_new += p_other.rad * form_factor(p, p_other, patches);
//            printf("no_int=%.0f+%.0f+%.0f int=%.0f\n", ni_1, ni_2, ni_3, pi);
        }

        rad_new *= p.color;
        rad_new += p.emit;

        p.rad_new = rad_new;
    }

    std::cout << vis / (vis + invis) * 100 << "% visbile" << std::endl;

    for (auto &p : patches) {
        p.rad = p.rad_new;
    }
}

void reinhard(std::vector<patch> &patches) {
    const float N = patches.size();
    const glm::vec3 a = glm::vec3(0.18f);
    glm::vec3 product = glm::vec3(1.0f);

    for (auto &p : patches) {
        product *= glm::vec3(
                p.rad.x == 0 ? 1.0f : std::pow(p.rad.x, 1.0f / N),
                p.rad.y == 0 ? 1.0f : std::pow(p.rad.y, 1.0f / N),
                p.rad.z == 0 ? 1.0f : std::pow(p.rad.z, 1.0f / N));
    }

    glm::vec3 L_avg = product;


    for (auto &p : patches) {
        p.rad = a / L_avg * p.rad;
    }

    for (auto &p : patches) {
        p.rad = p.rad / (glm::vec3(1.0f) + p.rad);

        p.rad.r = glm::pow(p.rad.r, (1.0f / 2.2f)); // gamma correction
        p.rad.g = glm::pow(p.rad.g, (1.0f / 2.2f));
        p.rad.b = glm::pow(p.rad.b, (1.0f / 2.2f));
    }

}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glLineWidth(2.0f);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    const glm::vec3 camera_pos(278.0f, 273.0f, -800.0f);
    const glm::vec3 camera_dir(0.0f, 0.0f, 1.0f);
    const glm::vec3 world_up(0.0f, 1.0f, 0.0);
    const glm::mat4 proj = glm::perspective(glm::radians(FOV), ASPECT_RATIO, 1.0f, 10000.0f); // replace with real fov
    const glm::mat4 view = glm::lookAt(camera_pos, camera_pos + camera_dir, world_up); // replace

    std::vector<patch> patches = load_mesh("models/big_triangles.obj");


    for (auto &p : patches) {
        p.rad = p.emit;
    }

    for (int i = 0; i < RAD_ITERATIONS; i++) {
        iteration(patches);
        std::cout << "Iteration " << i + 1 << " complete" << std::endl;
    }

    reinhard(patches);

    std::vector<GLfloat> vertices = {};

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

    GLuint VAO, VBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *) 0);
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *) (3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1); // color

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    utils::shader shader("GLSL/pass_3d.vert", "GLSL/white.frag");

    shader.use_program();
    shader.set_uniform<glm::mat4>("proj", proj);
    shader.set_uniform<glm::mat4>("view", view);

    while (glfwWindowShouldClose(window) == 0) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}