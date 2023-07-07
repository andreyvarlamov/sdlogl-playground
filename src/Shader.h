#ifndef SHADER_H
#define SHADER_H

#include "Common.h"

u32
BuildShaderProgram(const char *VertexPath, const char *FragmentPath);

void
SetUniformInt(u32 Shader, const char *UniformName, bool UseProgram, i32 Value);
void
SetUniformVec3F(u32 Shader, const char *UniformName, bool UseProgram, f32 *Value);
void
SetUniformMat3F(u32 Shader, const char *UniformName, bool UseProgram, f32 *Value);
void
SetUniformMat4F(u32 Shader, const char *UniformName, bool UseProgram, f32 *Value);

#endif