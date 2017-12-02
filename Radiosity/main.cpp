#include "shader.h"
#include "utils.h"
#include "radiosity.h"

#include <GLFW/glfw3.h>

/*
 * TODO:
 *
 * 1. вынести все функции в файл, чтобы не перекомпилить ++
 * 2. вынести все константы, чтобы не перекомилить ++
 * 3. нормальный тон-мап
 * 4. живая камера
 * 5. параметры по кнопкам (яркость)
 *
 * */

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    settings s = load_settings("constants");

    GLFWwindow *window = glfwCreateWindow(s.WINDOW_WIDTH, s.WINDOW_HEIGHT, "", nullptr, nullptr);
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
    const glm::mat4 proj = glm::perspective(glm::radians(s.FOV), s.ASPECT_RATIO, 1.0f, 10000.0f);
    const glm::mat4 view = glm::lookAt(camera_pos, camera_pos + camera_dir, world_up); // replace


    std::vector<patch> patches;

//    patches = load_mesh("models/cornell_tesselated.obj");
    auto obj = load_mesh("models/cornell_box/back_wall.obj");
    patches.insert(patches.end(), obj.begin(), obj.end());

    obj = load_mesh("models/cornell_box/light.obj");
    patches.insert(patches.end(), obj.begin(), obj.end());

    obj = load_mesh("models/cornell_box/ceiling.obj");
    patches.insert(patches.end(), obj.begin(), obj.end());

    obj = load_mesh("models/cornell_box/floor.obj");
    patches.insert(patches.end(), obj.begin(), obj.end());

    obj = load_mesh("models/cornell_box/red_wall.obj");
    patches.insert(patches.end(), obj.begin(), obj.end());

    obj = load_mesh("models/cornell_box/green_wall.obj");
    patches.insert(patches.end(), obj.begin(), obj.end());

    obj = load_mesh("models/cornell_box/tall_block.obj");
    patches.insert(patches.end(), obj.begin(), obj.end());

    obj = load_mesh("models/cornell_box/short_block.obj");
    patches.insert(patches.end(), obj.begin(), obj.end());

    double start = glfwGetTime();

    for (auto &p : patches) {
        p.rad = p.emit;
    }

    for (int i = 0; i < s.RAD_ITERATIONS; i++) {
        iteration(patches, s.ERR, s.FF_SAMPLES);
        std::cout << "Iteration " << i + 1 << " complete" << std::endl;
    }

    reinhard(patches);

    double seconds = glfwGetTime() - start;
    auto minutes = static_cast<int>(seconds / 60);
    seconds = (int) seconds % 60;

    std::cout << minutes << "m " << (int) seconds << "s" << std::endl;

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