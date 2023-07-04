#version 330 core
layout (location = 0) in vec3 In_Position;
layout (location = 1) in vec2 In_UVs;
layout (location = 2) in vec3 In_Normal;
layout (location = 3) in vec3 In_Tangent;
layout (location = 4) in vec3 In_Bitangent;
layout (location = 5) in ivec4 In_BoneIDs;
layout (location = 6) in vec4 In_BoneWeights;

out vertex_shader_out
{
    vec2 UVs;
    vec3 LightDirectionTangentSpace;
    vec3 ViewPositionTangentSpace;
    vec3 FragmentPositionTangentSpace;
} Out;

uniform mat4 Projection;
uniform mat4 View;
uniform mat4 Model;

#define MAX_BONES 128
uniform mat4 BoneTransforms[MAX_BONES];

uniform vec3 LightDirection;
uniform vec3 ViewPosition;

void main()
{
    mat4 boneTransform = mat4(0.0);
    for (int i = 0; i < 4; ++i)
    {
        // TODO: This if might not be good
        if (In_BoneIDs[i] > 0)
        {
            // TODO: The lerp should happen after bone transform is applied to position
            //       But cannot do that because there's scaling and that needs to be
            //       taken account of in the inverse normal transform.
            //       This seems to sort of work for now, so fix that when scaling is removed from animations.
            boneTransform += BoneTransforms[In_BoneIDs[i]] * In_BoneWeights[i];
        }
    }

    vec4 transformedPosition = Model * boneTransform * vec4(In_Position, 1.0);
    gl_Position = Projection * View * transformedPosition;
    Out.UVs = In_UVs;

    // TODO: Avoid scaling in animations, so there's no need to do this for every vertex
    mat3 normalMatrix = mat3(transpose(inverse(Model * boneTransform)));
    vec3 tangent = normalize(normalMatrix * In_Tangent);
    vec3 bitangent = normalize(normalMatrix * In_Bitangent);
    vec3 normal = normalize(normalMatrix * In_Normal);
    tangent = normalize(tangent - dot(tangent, normal) * normal);

    mat3 tbn = transpose(mat3(tangent, bitangent, normal));
    Out.LightDirectionTangentSpace = tbn * LightDirection;
    Out.ViewPositionTangentSpace = tbn * ViewPosition;
    Out.FragmentPositionTangentSpace = tbn * vec3(transformedPosition);
}
