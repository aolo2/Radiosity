#include "../includes/shader.h"
#include "../includes/camera.h"
#include "../includes/utils.h"
#include "../includes/radiosity.h"
#include "../includes/bvh.h"

#include <GLFW/glfw3.h>

camera *cam;
bool keys[1024] = {};
bool cam_interactive = false;
bool first_call = true;
double last_x, last_y;

std::atomic<bool> finished_radiosity(false);

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

    if (cam_interactive) {
        cam->process_cursor(x_offset, y_offset, true);
    }
}

/* Key press or release fires this callback */
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        keys[key] = true;
        switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GL_TRUE);
                break;
            case GLFW_KEY_SPACE:
                cam_interactive = !cam_interactive;
                glfwSetInputMode(window, GLFW_CURSOR,
                                 cam_interactive ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

//                std::cout << "Interactive camera mode "
//                          << (cam_interactive ? "ENABLED" : "DISABLED")
//                          << std::endl;
            default:
                break;
        }
    } else if (action == GLFW_RELEASE) {
        keys[key] = false;
    }
}

/* Per-frame updates */
void update(const utils::shader &s) {
    if (cam_interactive) {
        cam->process_movement(keys); // returns true if matrix was updated
    }

    s.set_uniform<glm::mat4>("view", cam->view_matrix());
}

void startup(std::vector<patch> &patches,
             std::vector<patch *> &primitives,
             std::vector<float> &vertices,
             bvh_node **tree,
             const settings &s,
             GLuint *VAO,
             GLuint *VBO) {

    std::cout << "Loading mesh... " << std::flush;
    patches = load_mesh(s.mesh_path);
    std::cout << "DONE" << std::endl;

    for (auto &patch : patches) {
        primitives.push_back(&patch);
    }

    std::cout << "Creating the BVH... " << std::flush;
    *tree = bvh(primitives);
    std::cout << "DONE" << std::endl;

    std::cout << "Initializing OpenGL buffers... " << std::flush;
    vertices = glify(primitives, true);
    init_buffers(VAO, VBO, vertices);
    std::cout << "DONE" << std::endl;
}

void radiate(std::vector<patch> &patches,
             std::vector<patch *> &primitives,
             std::vector<float> &vertices,
             bvh_node **tree,
             const settings &s) {


    /* Local line radiosity */
    std::cout << "Monte Carlo radiosity... " << std::endl;
    local_line(primitives, s.TOTAL_RAYS, *tree, s.ERR);

    /* Interpolate */
    std::cout << "Interpolating" << std::flush;
    interpolate(primitives, *tree, s.GATHER_RAYS, s.SHADOW_RAYS, s.ERR);
    std::cout << " DONE" << std::endl;

    /* Tone map */
    std::cout << "Tone mapping... " << std::flush;
    reinhard(primitives);
    std::cout << "DONE" << std::endl;

    /* Transform to OpenGL per-vertex format */
    std::cout << "Updating OpenGL buffers... " << std::flush;
    vertices = glify(primitives, false);
    finished_radiosity = true;
    std::cout << "DONE" << std::endl;
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
    settings s = load_settings("includes/constants");

    /* Create a window and set callbacks, etc. */
    GLFWwindow *window = glfwCreateWindow(s.WINDOW_WIDTH, s.WINDOW_HEIGHT, "", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetCursorPos(window, s.WINDOW_WIDTH / 2, s.WINDOW_HEIGHT / 2);

    /* Init OpenGL(GLEW) */
    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_DEPTH_TEST);
//    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.0f);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    /* Init view and projection matrices */
    const glm::vec3 world_up(0.0f, 1.0f, 0.0);
    cam = new camera(s.camera_pos, world_up);

    const glm::mat4 proj = glm::perspective(glm::radians(s.FOV), s.ASPECT_RATIO, 1.0f, 10000.0f);
    glm::mat4 view = cam->view_matrix();

    /* Create a shader program and init uniform variables */
    utils::shader shader("glsl/pass_3d.vert", "glsl/white.frag");
    shader.use_program();
    shader.set_uniform<glm::mat4>("proj", proj);
    shader.set_uniform<glm::mat4>("view", view);

    GLuint VAO, VBO;

    std::vector<patch> patches;
    std::vector<patch *> primitives;
    std::vector<float> vertices;
    bvh_node *tree;

    startup(patches, primitives, vertices, &tree, s, &VAO, &VBO);

    std::thread t1(radiate,
                   std::ref(patches),
                   std::ref(primitives),
                   std::ref(vertices),
                   &tree, s);

    /* Main draw loop */
    while (glfwWindowShouldClose(window) == 0) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        update(shader);

        if (finished_radiosity) {
            update_buffers(&VAO, &VBO, vertices);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            finished_radiosity = false;
        }

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }


    t1.join();

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