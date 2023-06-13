#version 330 core
out vec4 FragColor;

in vec3 Color;
in vec2 TexCoords;

uniform sampler2D tex;

void main()
{
    vec4 texColor = texture(tex, TexCoords);
    FragColor = texColor * vec4(Color, 1.0);
}