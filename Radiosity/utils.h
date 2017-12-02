#ifndef RADIOSITY_UTILS_H
#define RADIOSITY_UTILS_H

#include "shared.h"

settings load_settings(const std::string &path);

std::vector<patch> load_mesh(const std::string &path);

#endif //RADIOSITY_UTILS_H
