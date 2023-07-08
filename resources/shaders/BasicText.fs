#version 330 core
out vec4 Out_FragColor;

in vertex_shader_out
{
    vec2 UVs;
} In;

uniform sampler2D Text;

void main()
{
    Out_FragColor = vec4(texture(Text, In.UVs));
}