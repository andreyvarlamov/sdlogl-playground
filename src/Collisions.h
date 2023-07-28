#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>

#include "Common.h"
#include "Shader.h"

struct cube
{
    glm::vec3 Vertices[8];
    glm::vec3 Scale;
    glm::vec3 Position;

    glm::mat4 TransientModelTransform;
    glm::vec3 TransientTransformedVertices[8];

    u32 VAO;
};

cube CubeA;
cube CubeB;

void
DEBUG_GetUnitCubeVerticesAndIndices(glm::vec3 *Out_Vertices, i32 *Out_Indices);
void
DEBUG_TransformVertices(glm::mat4 Transform, glm::vec3 *Vertices, glm::vec3 *Out_TransformedVertices, i32 VertexCount);
f32 *
DEBUG_GetRawVertexDataFromVec3(glm::vec3 *Vertex, i32 VertexCount);
cube
DEBUG_GenerateCube(glm::vec3 Position, glm::vec3 Scale);
void
DEBUG_UpdateCube(cube *Cube);
void
DEBUG_RenderCube(u32 Shader, cube Cube);

void
DEBUG_CollisionTestSetup(u32 Shader)
{
    CubeA = DEBUG_GenerateCube(glm::vec3(4.0f, 1.0f, -2.0f), glm::vec3(2.0f, 1.0f, 1.0f));
    CubeB = DEBUG_GenerateCube(glm::vec3(4.0f, 1.0f, 2.0f), glm::vec3(1.0f, 1.0f, 2.0f));
}

void
DEBUG_CollisionTestUpdate(u32 Shader, f32 DeltaTime, glm::mat4 Projection, glm::mat4 View)
{
    // 1. Update positions, transforms, calculate vertices
    DEBUG_UpdateCube(&CubeA);
    DEBUG_UpdateCube(&CubeB);

    // 2. Collision detection

    // 3. Collision resolution, update positions, transforms

    // 4. Render
    UseShader(Shader);
    SetUniformMat4F(Shader, "Projection", false, glm::value_ptr(Projection));
    SetUniformMat4F(Shader, "View", false, glm::value_ptr(View));

    DEBUG_RenderCube(Shader, CubeA);
    DEBUG_RenderCube(Shader, CubeB);
}

f32 *
DEBUG_GetRawVertexDataFromVec3(glm::vec3 *Vertex, i32 VertexCount)
{
    i32 VectorSize = 3;

    f32 *Result = (f32 *) malloc(VectorSize * VertexCount * sizeof(f32));
    f32 *ResultCursor = Result;

    for (i32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
    {
        for (i32 Index = 0; Index < VectorSize; ++Index)
        {
            *ResultCursor++ = Vertex[VertexIndex][Index];
        }
    }

    return Result;
}


void
DEBUG_GetUnitCubeVerticesAndIndices(glm::vec3 *Out_Vertices, i32 *Out_Indices)
{
    if (Out_Vertices)
    {
        Out_Vertices[0].x = -0.5f; Out_Vertices[0].y =  0.5f; Out_Vertices[0].z =  0.5f;
        Out_Vertices[1].x = -0.5f; Out_Vertices[1].y = -0.5f; Out_Vertices[1].z =  0.5f;
        Out_Vertices[2].x =  0.5f; Out_Vertices[2].y = -0.5f; Out_Vertices[2].z =  0.5f;
        Out_Vertices[3].x =  0.5f; Out_Vertices[3].y =  0.5f; Out_Vertices[3].z =  0.5f;

        Out_Vertices[4].x =  0.5f; Out_Vertices[4].y =  0.5f; Out_Vertices[4].z = -0.5f;
        Out_Vertices[5].x =  0.5f; Out_Vertices[5].y = -0.5f; Out_Vertices[5].z = -0.5f;
        Out_Vertices[6].x = -0.5f; Out_Vertices[6].y = -0.5f; Out_Vertices[6].z = -0.5f;
        Out_Vertices[7].x = -0.5f; Out_Vertices[7].y =  0.5f; Out_Vertices[7].z = -0.5f;
    }

    if (Out_Indices)
    {
        i32 Indices[] = { 
            0, 1, 3,  3, 1, 2, // front
            4, 5, 7,  7, 5, 6, // back
            7, 0, 4,  4, 0, 3, // top
            1, 6, 2,  2, 6, 5, // bottom
            7, 6, 0,  0, 6, 1, // left
            3, 2, 4,  4, 2, 5  // right
        };

        memcpy_s(Out_Indices, 36 * sizeof(i32), Indices, 36 * sizeof(i32));
    }
}

void
DEBUG_TransformVertices(glm::mat4 Transform, glm::vec3 *Vertices, glm::vec3 *Out_TransformedVertices, i32 VertexCount)
{
    for (i32 Index = 0; Index < VertexCount; ++Index)
    { 
        glm::vec4 Transformed = Transform * glm::vec4(Out_TransformedVertices[Index], 1.0f);

        Out_TransformedVertices[Index].x = Transformed.x;
        Out_TransformedVertices[Index].y = Transformed.y;
        Out_TransformedVertices[Index].z = Transformed.x;
    }
}

cube
DEBUG_GenerateCube(glm::vec3 Position, glm::vec3 Scale)
{
    glm::vec3 CubeVertices[8];
    i32 CubeIndices[36];
    DEBUG_GetUnitCubeVerticesAndIndices(CubeVertices, CubeIndices);

    f32 *CubeRawData = DEBUG_GetRawVertexDataFromVec3(CubeVertices, 8);
    u32 CubeVAO;
    glGenVertexArrays(1, &CubeVAO);
    u32 CubeVBO;
    glGenBuffers(1, &CubeVBO);
    u32 CubeEBO;
    glGenBuffers(1, &CubeEBO);
    glBindVertexArray(CubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, CubeVBO);
    glBufferData(GL_ARRAY_BUFFER, 8 * 3 * sizeof(f32), CubeRawData, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, CubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(f32), CubeIndices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (void *) 0);
    glBindVertexArray(0);
    free(CubeRawData);

    cube Result;
    memcpy_s(Result.Vertices, 8 * sizeof(glm::vec3), CubeVertices, 8 * sizeof(glm::vec3));
    Result.Scale = Scale;
    Result.Position = Position;
    Result.VAO = CubeVAO;
    return Result;
}

void
DEBUG_UpdateCube(cube *Cube)
{
    glm::mat4 Model(1.0f);
    Model = glm::translate(Model, Cube->Position);
    Model = glm::scale(Model, Cube->Scale);
    Cube->TransientModelTransform = Model;
    DEBUG_TransformVertices(Model, Cube->Vertices, Cube->TransientTransformedVertices, 8);
}

void
DEBUG_RenderCube(u32 Shader, cube Cube)
{
    SetUniformMat4F(Shader, "Model", false, glm::value_ptr(Cube.TransientModelTransform));

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(3);

    glBindVertexArray(Cube.VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

#endif