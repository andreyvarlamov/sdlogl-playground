#ifndef UTIL_H
#define UTIL_H

#include "Common.h"
#include <string>

u32 loadTexture(const char* path);
std::string readFile(const char *path);
u32 buildShader(const char *vertexPath, const char *fragmentPath);

#endif