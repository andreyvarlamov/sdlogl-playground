#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in ivec4 aBoneIDs;
layout (location = 2) in vec4 aBoneWeights;

flat out ivec4 BoneIDs;
out vec4 BoneWeights;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(aPosition, 1.0f);

    BoneIDs = aBoneIDs;
    BoneWeights = aBoneWeights;
}
