#version 330 core

out vec4 Out_FragColor;

uniform vec3 Color;

void main()
{
    Out_FragColor = vec4(Color, 1.0);
}