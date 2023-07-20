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
    if (texel.a < 0.1)
    {
//        discard;
        Out_FragColor = vec4(0.5, 0.5, 0.7, 1.0);
    }
    else
    {
        Out_FragColor = texel;
    }
//    Out_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}