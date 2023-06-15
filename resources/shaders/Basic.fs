#version 330 core
out vec4 FragColor;

in VS_OUT
{
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D diffuseMap;
uniform sampler2D specularMap;
uniform sampler2D emissionMap;
uniform sampler2D normalMap;

uniform vec3 lightDirection;
uniform vec3 viewPosition;

void main()
{
    vec3 normal = normalize(fs_in.Normal);
    vec3 fragColor = texture(diffuseMap, fs_in.TexCoords).rgb;
    vec3 specularColorSample = texture(specularMap, fs_in.TexCoords).rgb;
    float fragBrightness = 0.0;

    // ambient
    // -------
    fragBrightness += 0.1;

    // diffuse
    // -------
    vec3 normalizedLightDirection = normalize(-lightDirection);
    float diffuseBrightness = max(dot(normalizedLightDirection, normal), 0.0);
    fragBrightness += diffuseBrightness;

    // specular
    // --------
    vec3 normalizedViewDirection = normalize(viewPosition - fs_in.FragPos);
    vec3 halfwayDirection = normalize(normalizedLightDirection + normalizedViewDirection);
    float specularBrightness = pow(max(dot(normal, halfwayDirection), 0.0), 128.0);
    vec3 specularColor = specularColorSample * specularBrightness;

    FragColor = vec4(fragBrightness * fragColor + specularColor, 1.0);
}