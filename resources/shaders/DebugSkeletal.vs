#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in ivec4 aBoneIDs;
layout (location = 2) in vec4 aBoneWeights;

out float BoneInfluence;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(aPosition, 1.0f);

    for (int i = 0; i < 4; ++i)
    {
        if (aBoneIDs[i] == 3)
        {
            BoneInfluence = aBoneWeights[i];
        }
    }
}
