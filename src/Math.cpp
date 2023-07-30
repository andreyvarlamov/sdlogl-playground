#include "Math.h"

f32
GetSquaredVectorLength(glm::vec3 Vector)
{
    return Vector.x * Vector.x + Vector.y * Vector.y + Vector.z * Vector.z;
}