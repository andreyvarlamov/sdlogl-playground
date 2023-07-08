#include "Shader.h"

#include <glad/glad.h>

#include <cstdlib>
#include <cstdio>

#include "Util.h"

static u32
CompileShaderAndCheckErrors(const char *Path, GLenum GLShaderType);
static u32
LinkShaderProgramAndCleanShaders(u32 *Shaders, i32 ShaderCount);

u32
BuildShaderProgram(const char *VertexPath, const char *FragmentPath)
{
    printf("Building shader program\n");
    printf("Vertex shader: %s\n", VertexPath);
    printf("Fragment shader: %s\n", FragmentPath);

    u32 VertexShader = CompileShaderAndCheckErrors(VertexPath, GL_VERTEX_SHADER);
    u32 FragmentShader = CompileShaderAndCheckErrors(FragmentPath, GL_FRAGMENT_SHADER);

    u32 Shaders[] = { VertexShader, FragmentShader };
    i32 ShaderCount = 2;
    u32 ShaderProgram = LinkShaderProgramAndCleanShaders(Shaders, ShaderCount);

    return ShaderProgram;
}

void
SetUniformInt(u32 Shader, const char *UniformName, bool UseProgram, i32 Value)
{
    if (UseProgram)
    {
        glUseProgram(Shader);
    }

    i32 UniformLocation = glGetUniformLocation(Shader, UniformName);
    Assert(UniformLocation != -1);

    glUniform1i(UniformLocation, Value);
}

void
SetUniformVec3F(u32 Shader, const char *UniformName, bool UseProgram, f32 *Value)
{
    if (UseProgram)
    {
        glUseProgram(Shader);
    }

    i32 UniformLocation = glGetUniformLocation(Shader, UniformName);
    Assert(UniformLocation != -1);

    glUniform3fv(UniformLocation, 1, Value);
}

void
SetUniformMat3F(u32 Shader, const char *UniformName, bool UseProgram, f32 *Value)
{
    if (UseProgram)
    {
        glUseProgram(Shader);
    }

    i32 UniformLocation = glGetUniformLocation(Shader, UniformName);
    Assert(UniformLocation != -1);

    glUniformMatrix3fv(UniformLocation, 1, GL_FALSE, Value);
}

void
SetUniformMat4F(u32 Shader, const char *UniformName, bool UseProgram, f32 *Value)
{
    if (UseProgram)
    {
        glUseProgram(Shader);
    }

    i32 UniformLocation = glGetUniformLocation(Shader, UniformName);
    Assert(UniformLocation != -1);

    glUniformMatrix4fv(UniformLocation, 1, GL_FALSE, Value);
}

static u32
CompileShaderAndCheckErrors(const char *Path, GLenum GLShaderType)
{
    u32 Shader;

    Shader = glCreateShader(GLShaderType);
    size_t SourceSize = 0;
    char *Source = ReadFile(Path, &SourceSize);
    glShaderSource(Shader, 1, &Source, 0);
    glCompileShader(Shader);
    i32 Success;
    char InfoLog[512];
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &Success);
    if (!Success)
    {
        glGetShaderInfoLog(Shader, 512, 0, InfoLog);
        fprintf(stderr, "Failed to compile shader: %s\n", InfoLog);
    }
    Assert(Success);

    free(Source);

    return Shader;
}

static u32
LinkShaderProgramAndCleanShaders(u32 *Shaders, i32 ShaderCount)
{
    u32 ShaderProgram;

    ShaderProgram = glCreateProgram();
    for (i32 ShaderIndex = 0; ShaderIndex < ShaderCount; ++ShaderIndex)
    {
        glAttachShader(ShaderProgram, Shaders[ShaderIndex]);
    }
    glLinkProgram(ShaderProgram);
    i32 Success;
    char InfoLog[512];
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);
    if (!Success)
    {
        glGetProgramInfoLog(ShaderProgram, 512, 0, InfoLog);
        fprintf(stderr, "Failed to link shader program: %s\n", InfoLog);
    }
    Assert(Success);

    for (i32 ShaderIndex = 0; ShaderIndex < ShaderCount; ++ShaderIndex)
    {
        glDeleteShader(Shaders[ShaderCount]);
    }

    return ShaderProgram;
}

void
UseShader(u32 Shader)
{
    glUseProgram(Shader);
}
