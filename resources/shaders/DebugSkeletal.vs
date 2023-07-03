#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aBoneWeights;

out VS_OUT
{
    vec2 TextureCoordinates;
    vec3 LightDirectionTangentSpace;
    vec3 ViewPositionTangentSpace;
    vec3 FragmentPositionTangentSpace;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform int selectedBone;

#define MAX_BONES 128
uniform mat4 boneTransforms[MAX_BONES];

uniform vec3 lightDirection;
uniform vec3 viewPosition;

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
    }

    if (!boneTransformToApply)
    {
        boneTransform = mat4(1.0);
    }
 
    gl_Position = projection * view * model * boneTransform * vec4(aPosition, 1.0);

    vs_out.TextureCoordinates = aUV;

    mat3 normalMatrix = mat3(transpose(inverse(model * boneTransform)));
    vec3 Tangent = normalize(normalMatrix * aTangent);
    vec3 Normal = normalize(normalMatrix * aNormal);
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    vec3 Bitangent = cross(Tangent, Normal);

    mat3 TBN = transpose(mat3(Tangent, Bitangent, Normal));
    vs_out.LightDirectionTangentSpace = TBN * lightDirection;
    vs_out.ViewPositionTangentSpace = TBN * viewPosition;
    vs_out.FragmentPositionTangentSpace = TBN * vec3(model * vec4(aPosition, 1.0));
}
