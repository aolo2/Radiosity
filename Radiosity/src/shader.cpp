#include "../includes/shader.h"

namespace utils {

    void shader::attach_shader(const std::string &path, GLenum stage) {
        GLuint shader;
        GLint success;
        GLchar info_log[512];

        std::ifstream t(path);
        std::string source_str((std::istreambuf_iterator<char>(t)),
                               std::istreambuf_iterator<char>());

        const char *source_char = source_str.c_str();

        shader = glCreateShader(stage);
        glShaderSource(shader, 1, &source_char, NULL);
        glCompileShader(shader);

        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, NULL, info_log);
            std::cerr << ((stage == GL_VERTEX_SHADER) ? "Vertex" : "Fragment") << " shader compilaton failed\n"
                      << info_log << std::endl;
        }

        glAttachShader(this->program, shader);
        glDeleteShader(shader);
    }

    void shader::link_program() {
        GLint success;
        GLchar info_log[512];

        glLinkProgram(this->program);

        glGetProgramiv(this->program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(this->program, 512, NULL, info_log);
            std::cerr << "Shader program linking failed\n"
                      << info_log << std::endl;
        }
    }

    shader::shader(const std::string &vert_path, const std::string &frag_path) {
        this->program = glCreateProgram();

        attach_shader(vert_path, GL_VERTEX_SHADER);
        attach_shader(frag_path, GL_FRAGMENT_SHADER);

        link_program();
    }

    void shader::use_program() const {
        glUseProgram(this->program);
    }

    template<>
    void shader::set_uniform<glm::mat4>(const std::string &var_string, const glm::mat4 &data) const {
        GLint loc = glGetUniformLocation(this->program, var_string.c_str());
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(data));
    }

    template<>
    void shader::set_uniform<glm::vec3>(const std::string &var_string, const glm::vec3 &data) const {
        GLint loc = glGetUniformLocation(this->program, var_string.c_str());
        glUniform3f(loc, data.x, data.y, data.z);
    }

    template<>
    void shader::set_uniform<glm::vec2>(const std::string &var_string, const glm::vec2 &data) const {
        GLint loc = glGetUniformLocation(this->program, var_string.c_str());
        glUniform2f(loc, data.x, data.y);
    }

    template<>
    void shader::set_uniform<float>(const std::string &var_string, const float &data) const {
        GLint loc = glGetUniformLocation(this->program, var_string.c_str());
        glUniform1f(loc, data);
    }

    template<>
    void shader::set_uniform<int>(const std::string &var_string, const int &data) const {
        GLint loc = glGetUniformLocation(this->program, var_string.c_str());
        glUniform1i(loc, data);
    }

    template<>
    void shader::set_uniform<glm::mat4>(const std::string &var_string, const glm::mat4 &&data) const {
        GLint loc = glGetUniformLocation(this->program, var_string.c_str());
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(data));
    }

    template<>
    void shader::set_uniform<glm::vec3>(const std::string &var_string, const glm::vec3 &&data) const {
        GLint loc = glGetUniformLocation(this->program, var_string.c_str());
        glUniform3f(loc, data.x, data.y, data.z);
    }

    template<>
    void shader::set_uniform<glm::vec2>(const std::string &var_string, const glm::vec2 &&data) const {
        GLint loc = glGetUniformLocation(this->program, var_string.c_str());
        glUniform2f(loc, data.x, data.y);
    }
}