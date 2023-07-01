#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 aNormals;
layout (location = 3) in vec3 aTangents;
layout (location = 4) in vec3 aBitangents;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aBoneWeights;

out float BoneInfluence;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform int selectedBone;

#define MAX_BONES 128
uniform mat4 boneTransforms[MAX_BONES];

void main()
{
    mat4 boneTransform = mat4(0.0);
    bool boneTransformToApply = false;
    for (int i = 0; i < 4; ++i)
    {
        if (aBoneIDs[i] > 0)
        {
            boneTransform += boneTransforms[aBoneIDs[i]] * aBoneWeights[i];
            boneTransformToApply = true;
        }

        if (aBoneIDs[i] == selectedBone)
        {
            BoneInfluence = aBoneWeights[i];
        }
    }

    if (!boneTransformToApply)
    {
        boneTransform = mat4(1.0);
    }
 
    gl_Position = projection * view * model * boneTransform * vec4(aPosition, 1.0);
//    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}
