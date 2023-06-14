#version 330 core
out vec4 FragColor;

in VS_OUT
{
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D diffuseMap;

uniform vec3 lightDirection;

void main()
{
    vec3 normal = normalize(fs_in.Normal);
    vec3 fragColor = texture(diffuseMap, fs_in.TexCoords).rgb;
    float fragBrightness = 0.0;

    // ambient
    // -------
    fragBrightness += 0.1;

    // diffuse
    // -------
    vec3 normalizedLightDirection = normalize(-lightDirection);
    float diffuseBrightness = max(dot(normalizedLightDirection, normal), 0.0);
    fragBrightness += diffuseBrightness;

    FragColor = vec4(fragBrightness * fragColor, 1.0);
}