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
    bool boneTransformToApply = false;
    for (int i = 0; i < 4; ++i)
    {
        if (aBoneIDs[i] > 0)
        {
            boneTransform += boneTransforms[aBoneIDs[i] - 1] * aBoneWeights[i];
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
}
