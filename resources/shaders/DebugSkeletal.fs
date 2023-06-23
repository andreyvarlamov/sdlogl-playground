#version 330 core
out vec4 FragColor;

in float BoneInfluence;

void main()
{
    vec3 color = vec3(BoneInfluence);
    FragColor = vec4(color, 1.0);
}