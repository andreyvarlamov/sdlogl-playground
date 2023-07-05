#version 330 core
out vec4 Out_FragColor;

in vertex_shader_out
{
    vec2 UVs;
    vec3 LightDirectionTangentSpace;
    vec3 ViewPositionTangentSpace;
    vec3 FragmentPositionTangentSpace;
} In;

uniform sampler2D DiffuseMap;
uniform sampler2D SpecularMap;
uniform sampler2D EmissionMap;
uniform sampler2D NormalMap;

void main()
{
    vec3 normalSample = texture(NormalMap, In.UVs).rgb;
    vec3 normal;
    // TODO: more efficient way to do this?
    if (normalSample.r > 0.0 || normalSample.g > 0.0 || normal.b > 0.0)
    {
        normal = normalize(normalSample * 2.0 - 1.0);
    }
    else
    {
        normal = vec3(0.0, 0.0, 1.0);
    }

    float fragBrightness = 0.0;

    // ambient
    // -------
    fragBrightness += 0.1;

    // diffuse
    // -------
    vec3 normalizedLightDirection = normalize(-In.LightDirectionTangentSpace);
    float diffuseBrightness = max(dot(normalizedLightDirection, normal), 0.0);
    fragBrightness += diffuseBrightness;

    // specular
    // --------
    vec3 normalizedViewDirection = normalize(In.ViewPositionTangentSpace - In.FragmentPositionTangentSpace);
    vec3 halfwayDirection = normalize(normalizedLightDirection + normalizedViewDirection);
    float specularBrightness = pow(max(dot(normal, halfwayDirection), 0.0), 128.0);
    vec3 specularColorSample = texture(SpecularMap, In.UVs).rgb;
    vec3 specularColor = specularColorSample * specularBrightness;

    // emission
    // --------
    // TODO: Confirm that when EmissionMap is 0, texture() will not attempt to sample
    vec3 emissionColor = texture(EmissionMap, In.UVs).rgb;

    // combine colors
    // --------------
    vec3 fragColor = texture(DiffuseMap, In.UVs).rgb;
    
    Out_FragColor = vec4(fragBrightness * fragColor + specularColor + emissionColor, 1.0);
}