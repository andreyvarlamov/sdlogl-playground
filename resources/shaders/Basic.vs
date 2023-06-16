#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTextureCoordinates;
layout (location = 3) in vec3 aTangent;

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
uniform mat3 normalMatrix;

uniform vec3 lightDirection;
uniform vec3 viewPosition;

void main()
{
    gl_Position = projection * view * model * vec4(aPosition, 1.0f);
    vs_out.TextureCoordinates = aTextureCoordinates;

    vec3 Tangent = normalize(normalMatrix * aTangent);
    vec3 Normal = normalize(normalMatrix * aNormal);
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    vec3 Bitangent = cross(Tangent, Normal);

    mat3 TBN = transpose(mat3(Tangent, Bitangent, Normal));
    vs_out.LightDirectionTangentSpace = TBN * lightDirection;
    vs_out.ViewPositionTangentSpace = TBN * viewPosition;
    vs_out.FragmentPositionTangentSpace = TBN * vec3(model * vec4(aPosition, 1.0));
}
