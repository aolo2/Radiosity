#include "shader.h"
#include "camera.h"
#include "utils.h"
#include "radiosity.h"
#include "bvh.h"

#include <GLFW/glfw3.h>

camera *cam;
bool keys[1024] = {};
bool cam_interactive = false;
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

                std::cout << "Interactive camera mode "
                          << (cam_interactive ? "ENABLED" : "DISABLED")
                          << std::endl;
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

    /* Init OpenGL(GLEW) */
    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glLineWidth(2.0f);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    /* Init view and projection matrices */
    const glm::vec3 world_up(0.0f, 1.0f, 0.0);
    cam = new camera(s.camera_pos, world_up);

    const glm::mat4 proj = glm::perspective(glm::radians(s.FOV), s.ASPECT_RATIO, 1.0f, 10000.0f);
    glm::mat4 view = cam->view_matrix();

    /* Create a shader program and init uniform variables */
    utils::shader shader("GLSL/pass_3d.vert", "GLSL/white.frag");
    shader.use_program();
    shader.set_uniform<glm::mat4>("proj", proj);
    shader.set_uniform<glm::mat4>("view", view);

    /* Load geometry from wavefront obj */
    std::vector<patch> patches = load_mesh(s.mesh_path);
    std::vector<patch *> primitives(patches.size());

    for (auto i = 0; i < patches.size(); i++) {
        primitives[i] = &patches[i];
    }

    bvh_node *tree = bvh(primitives);

#ifdef DEBUG
    //    GLuint BVH_VAO, BVH_VBO;
    //    std::vector<float> bvh_verts = bvh_debug_vertices(tree, 0);
    //    init_buffers(&BVH_VAO, &BVH_VBO, bvh_verts);

#endif


    /* Start keeping the time */
    double start = glfwGetTime();
    double seconds, minutes;

    std::vector<object> objects;

    /* (-1)th iteration of radiosity  */
    for (auto &o : objects) {
        for (auto &p : o.patches) {
            p.rad = p.emit;
        }
    }


#ifdef LOCAL

#ifdef RAYS
    GLuint iVAO, iVBO;
    std::vector<float> Ivertices(glify(primitives));
    init_buffers(&iVAO, &iVBO, Ivertices);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(iVAO);
    glDrawArrays(GL_TRIANGLES, 0, Ivertices.size() / 3);
    glBindVertexArray(0);
    glfwSwapBuffers(window);
//    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

    /* Local line radiosity */
    local_line(primitives, s.TOTAL_RAYS, tree,
#ifdef RAYS
               window,
               iVAO,
               Ivertices.size(),
#endif

               s.ERR);
    std::cout << "LOCAL LINES DONE" << std::endl;
#else
    /* Jacobi iterations */
    for (int i = 0; i < s.RAD_ITERATIONS; i++) {
        iteration(tree, primitives, s.ERR, s.FF_SAMPLES);
        std::cout << "Iteration " << i + 1 << "/" << s.RAD_ITERATIONS << " complete" << std::endl;
    }
#endif

    /* Tone map */
    reinhard(primitives);

    /* Display computation time */
    seconds = (glfwGetTime() - start) / s.RAD_ITERATIONS / s.FF_SAMPLES;
    minutes = static_cast<int>(seconds / 60);
    seconds = seconds - minutes * 60;
//    std::cout << minutes << "m " << seconds << "s per iteration per FF sample" << std::endl;

    /* Transform to OpenGL per-vertex format */
    std::vector<float> vertices(glify(primitives));

    /* Init OpenGL buffers */
    GLuint VAO, VBO;
    init_buffers(&VAO, &VBO, vertices);

    /* Main draw loop */
    while (glfwWindowShouldClose(window) == 0) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        update(shader);

#if 0
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glEnable(GL_CULL_FACE);
        glBindVertexArray(BVH_VAO);
        glDrawArrays(GL_TRIANGLES, 0, bvh_verts.size() / 3);
        glBindVertexArray(0);
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

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