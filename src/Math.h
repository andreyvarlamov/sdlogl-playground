#ifndef MATH_H
#define MATH_H

#include <glm/glm.hpp>

#include "Common.h"

f32
GetSquaredVectorLength(glm::vec3 Vector);

bool
ClampVec3(glm::vec3 *Vector, glm::vec3 Min, glm::vec3 Max);

#endif