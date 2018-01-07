#ifndef RADIOSITY_SHADER_H
#define RADIOSITY_SHADER_H

#define GLEW_STATIC

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

namespace utils {

    class shader {

    public:
        template<typename T>
        void set_uniform(const std::string &var_string, const T &data) const;

        template<typename T>
        void set_uniform(const std::string &var_string, const T &&data) const;

        void use_program() const;

        shader(const shader &other) = delete;

        shader &operator=(shader other) = delete;

        shader &operator=(shader &&other) = delete;

        shader(shader &&other) = delete;

        shader(const std::string &vert_path, const std::string &frag_path);

    private:
        GLuint program;

        void attach_shader(const std::string &path, GLenum stage);

        void link_program();
    };

}


#endif //RADIOSITY_SHADER_H
