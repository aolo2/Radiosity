#ifndef RADIOSITY_UTILS_H
#define RADIOSITY_UTILS_H

#include "shared.h"
#include "bvh.h"
#include "stats.h"

const std::set<std::string> ALLOWED_FLAGS{"-stats", "-l", "-s", "-v", "-d"};

void load_settings(const std::string &path, settings &s);

settings process_flags(int argc, char **argv);

std::vector<patch> load_mesh(const std::string &path, stats &stat);

std::vector<float> glify(const std::vector<patch *> &primitives, bool fill);

void init_buffers(GLuint *VAO, GLuint *VBO, std::vector<float> &vertices);

void update_buffers(GLuint *VAO, GLuint *VBO, std::vector<float> &vertices);

#endif //RADIOSITY_UTILS_H
