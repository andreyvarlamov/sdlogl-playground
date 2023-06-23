#version 330 core
out vec4 FragColor;

flat in ivec4 BoneIDs;
in vec4 BoneWeights;

void main()
{
    vec3 color = vec3(0.0);
    for (int i = 0; i < 4; ++i)
    {
        if (BoneIDs[i] == 3)
        {
            color = vec3(BoneWeights[i]);
        }
    }
    FragColor = vec4(color, 1.0);
}