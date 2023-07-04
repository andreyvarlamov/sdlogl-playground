#version 330 core
layout (location = 0) in vec3 In_Position;
// TODO: Fix order once model loading is centralized
layout (location = 1) in vec3 In_Normal;
layout (location = 2) in vec2 In_UVs;
layout (location = 3) in vec3 In_Tangent;
//layout (location = 4) in vec3 In_Bitangent;

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

uniform vec3 LightDirection;
uniform vec3 ViewPosition;

void main()
{
    vec4 transformedPosition = Model * vec4(In_Position, 1.0);
    gl_Position = Projection * View * transformedPosition;
    Out.UVs = In_UVs;

    // TODO: Once skeletal animation scaling is gone, do this on CPU once per frame 
    mat3 normalMatrix = mat3(transpose(inverse(Model)));
    vec3 tangent = normalize(normalMatrix * In_Tangent);
    // TODO: Once models are used for primitives, bitangent should already be calculated
//    vec3 bitangent = normalize(normalMatrix * In_Bitangent);
    vec3 normal = normalize(normalMatrix * In_Normal);
    tangent = normalize(tangent - dot(tangent, normal) * normal);
    vec3 bitangent = cross(tangent, normal);

    mat3 tbn = transpose(mat3(tangent, bitangent, normal));
    Out.LightDirectionTangentSpace = tbn * LightDirection;
    Out.ViewPositionTangentSpace = tbn * ViewPosition;
    Out.FragmentPositionTangentSpace = tbn * vec3(transformedPosition);
}
