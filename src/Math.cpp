#include "Math.h"

f32
GetSquaredVectorLength(glm::vec3 Vector)
{
    return Vector.x * Vector.x + Vector.y * Vector.y + Vector.z * Vector.z;
}

bool
ClampVec3(glm::vec3 *Vector, glm::vec3 Min, glm::vec3 Max)
{
    Assert(Vector);

    bool DidClamp = false;

    for (i32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        if ((*Vector)[AxisIndex] > Max[AxisIndex])
        {
            (*Vector)[AxisIndex] = Max[AxisIndex];
            DidClamp = true;
        }
        else if ((*Vector)[AxisIndex] < Min[AxisIndex])
        {
            (*Vector)[AxisIndex] = Min[AxisIndex];
            DidClamp = true;
        }
    }

    return DidClamp;
}