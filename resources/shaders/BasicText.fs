#version 330 core
out vec4 Out_FragColor;

in vertex_shader_out
{
    vec2 UVs;
} In;

uniform sampler2D FontAtlas;

void main()
{
    vec4 texel = texture(FontAtlas, In.UVs);
    if (texel.a < 0.01)
    {
        discard;    
    }
    Out_FragColor = texel;
}