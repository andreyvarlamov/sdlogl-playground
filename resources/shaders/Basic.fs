#version 330 core
out vec4 FragColor;

in VS_OUT
{
    vec2 TextureCoordinates;
    vec3 LightDirectionTangentSpace;
    vec3 ViewPositionTangentSpace;
    vec3 FragmentPositionTangentSpace;
} fs_in;

uniform sampler2D diffuseMap;
uniform sampler2D specularMap;
uniform sampler2D emissionMap;
uniform sampler2D normalMap;

void main()
{
    vec3 normal = texture(normalMap, fs_in.TextureCoordinates).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    float fragBrightness = 0.0;

    // ambient
    // -------
    fragBrightness += 0.1;

    // diffuse
    // -------
    vec3 normalizedLightDirection = normalize(-fs_in.LightDirectionTangentSpace);
    float diffuseBrightness = max(dot(normalizedLightDirection, normal), 0.0);
    fragBrightness += diffuseBrightness;

    // specular
    // --------
    vec3 normalizedViewDirection = normalize(fs_in.ViewPositionTangentSpace - fs_in.FragmentPositionTangentSpace);
    vec3 halfwayDirection = normalize(normalizedLightDirection + normalizedViewDirection);
    float specularBrightness = pow(max(dot(normal, halfwayDirection), 0.0), 128.0);
    vec3 specularColorSample = texture(specularMap, fs_in.TextureCoordinates).rgb;
    vec3 specularColor = specularColorSample * specularBrightness;

    // emission
    // --------
    vec3 emissionColor = texture(emissionMap, fs_in.TextureCoordinates).rgb;

    // combine colors
    // --------------
    vec3 fragColor = texture(diffuseMap, fs_in.TextureCoordinates).rgb;
    
    FragColor = vec4(fragBrightness * fragColor + specularColor + emissionColor, 1.0);
}