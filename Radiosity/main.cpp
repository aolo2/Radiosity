#include "shader.h"
#include "camera.h"
#include "utils.h"
#include "radiosity.h"

#include <GLFW/glfw3.h>

camera *cam;
bool keys[1024] = {};
bool first_call = true;
double last_x, last_y;

/* Cursor movement fires this callback */
void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) {
    double x_offset = 0.0, y_offset = 0.0;

    if (!first_call) {
        x_offset = xpos - last_x;
        y_offset = last_y - ypos; // window coordiantes are inverted!
        last_x = xpos;
        last_y = ypos;
    } else {
        last_x = xpos;
        last_y = ypos;
        first_call = false;
    }

    cam->process_cursor(x_offset, y_offset, true);
}

/* Key press or release fires this callback */
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GL_TRUE);
                break;
            default:
                keys[key] = true;
                break;
        }
    } else if (action == GLFW_RELEASE) {
        keys[key] = false;
    }
}

/* Per-frame updates */
void update(const utils::shader &s) {
    cam->process_movement(keys); // returns true if matrix was updated
    s.set_uniform<glm::mat4>("view", cam->view_matrix());
}

int main() {
    /* Init glfw */
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    /* Load constants */
    settings s = load_settings("constants");

    /* Create a window and set callbacks, etc. */
    GLFWwindow *window = glfwCreateWindow(s.WINDOW_WIDTH, s.WINDOW_HEIGHT, "", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetCursorPos(window, s.WINDOW_WIDTH / 2, s.WINDOW_HEIGHT / 2);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    /* Init OpenGL(GLEW) */
    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glLineWidth(2.0f);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    /* Init view and projection matrices */
    const glm::vec3 world_up(0.0f, 1.0f, 0.0);
    cam = new camera(s.camera_pos, world_up);

    const glm::mat4 proj = glm::perspective(glm::radians(s.FOV), s.ASPECT_RATIO, 1.0f, 10000.0f);
    glm::mat4 view = cam->view_matrix();

    /* Load geometry from wavefront obj */
    std::vector<object> objects = load_objects("models/cornell_box/");

    /* Start keeping the time */
    double start = glfwGetTime();
    double seconds, minutes;

    /* (-1)th iteration of radiosity  */
    for (auto &o : objects) {
        for (auto &p : o.patches) {
            p.rad = p.emit;
        }
    }

    /* Jacobi iterations */
    for (int i = 0; i < s.RAD_ITERATIONS; i++) {
        iteration(objects, s.ERR, s.FF_SAMPLES);
        std::cout << "Iteration " << i + 1 << " complete" << std::endl;
    }

    /* Tone map */
    reinhard(objects);

    /* Display computation time */
    seconds = glfwGetTime() - start;
    minutes = static_cast<int>(seconds / 60);
    seconds = (int) seconds % 60;
    std::cout << minutes << "m " << (int) seconds << "s" << std::endl;

    /* Transform to OpenGL per-vertex format */
    std::vector<float> vertices(glify(objects));

    /* Init OpenGL buffers */
    GLuint VAO, VBO;
    init_buffers(&VAO, &VBO, vertices);

    /* Create a shader program and init uniform variables */
    utils::shader shader("GLSL/pass_3d.vert", "GLSL/white.frag");
    shader.use_program();
    shader.set_uniform<glm::mat4>("proj", proj);
    shader.set_uniform<glm::mat4>("view", view);


    /* Main draw loop */
    while (glfwWindowShouldClose(window) == 0) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        update(shader);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }


    /* Clean up */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);

    glfwDestroyWindow(window);
    glfwTerminate();

    delete cam;

    return 0;
}