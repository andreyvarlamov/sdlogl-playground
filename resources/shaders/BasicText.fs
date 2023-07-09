#version 330 core
out vec4 Out_FragColor;

in vertex_shader_out
{
    vec2 UVs;
} In;

uniform sampler2D Text;

void main()
{
    vec4 texel = texture(Text, In.UVs);
    if (texel.a < 0.1)
    {
        discard;
    }
    Out_FragColor = texel;
}