#ifndef RADIOSITY_UTILS_H
#define RADIOSITY_UTILS_H

#include "shared.h"
#include "bvh.h"

settings load_settings(const std::string &path);

std::vector<patch> load_mesh(const std::string &path);

std::vector<float> glify(const std::vector<patch> &patches);

void init_buffers(GLuint *VAO, GLuint *VBO, const std::vector<float> &vertices);

#endif //RADIOSITY_UTILS_H
