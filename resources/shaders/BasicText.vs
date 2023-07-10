#version 330 core
layout (location = 0) in vec2 In_Position;
layout (location = 1) in vec2 In_UVs;

uniform mat4 TextScale;

out vertex_shader_out
{
    vec2 UVs;
} Out;

void main()
{
    gl_Position = TextScale * vec4(In_Position, 0.0, 1.0);
    Out.UVs = In_UVs;
}
