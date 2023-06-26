#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in ivec4 aBoneIDs;
layout (location = 2) in vec4 aBoneWeights;

out float BoneInfluence;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform int selectedBone;

uniform mat4 boneTransforms[10];

void main()
{
    mat4 boneTransform = mat4(0.0);
    for (int i = 0; i < 4; ++i)
    {
        boneTransform += boneTransforms[aBoneIDs[i]] * aBoneWeights[i];

        if (aBoneIDs[i] == selectedBone)
        {
            BoneInfluence = aBoneWeights[i];
        }
    }
 
    gl_Position = projection * view * model * boneTransform * vec4(aPosition, 1.0f);
}
