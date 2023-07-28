#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cfloat>
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
    glm::vec3 PositionError;
    bool IsColliding;

    u32 VAO;
};

glm::vec3 ShapeColorNoCollision(0.8f, 0.8f, 0.0f);
glm::vec3 ShapeColorCollision(1.0f, 0.7f, 0.7f);

cube CubeStatic;
cube CubeMoving;

void
DEBUG_GetUnitCubeVerticesAndIndices(glm::vec3 *Out_Vertices, i32 *Out_Indices);
void
DEBUG_TransformVertices(glm::mat4 Transform, glm::vec3 *Vertices, glm::vec3 *Out_TransformedVertices, i32 VertexCount);
f32 *
DEBUG_GetRawVertexDataFromVec3(glm::vec3 *Vertex, i32 VertexCount);
cube
DEBUG_GenerateCube(glm::vec3 Position, glm::vec3 Scale);
void
DEBUG_MoveCube(cube *Cube, f32 DeltaTime, glm::vec3 Velocity);
void
DEBUG_ProcessCubePosition(cube *Cube);
void
DEBUG_CheckCollisions(cube *CubeA, cube *CubeB);
void
DEBUG_GetAABB(cube *Cube, glm::vec3 *Out_Min, glm::vec3 *Out_Max, glm::vec3 *Out_Center, glm::vec3 *Out_Extents);
void
DEBUG_RenderCube(u32 Shader, cube *Cube);

void
DEBUG_CollisionTestSetup(u32 Shader)
{
    CubeStatic = DEBUG_GenerateCube(glm::vec3(4.0f, 1.0f, -2.0f), glm::vec3(2.0f, 1.0f, 1.0f));
    CubeMoving = DEBUG_GenerateCube(glm::vec3(4.0f, 1.0f, 2.0f), glm::vec3(1.0f, 1.0f, 2.0f));
}

void
DEBUG_CollisionTestUpdate(u32 Shader, f32 DeltaTime,
                          glm::mat4 Projection, glm::mat4 View,
                          glm::vec3 PlayerCubeVelocity,
                          glm::vec3 *Out_PlayerCubePosition)
{
    // 1. Update moving cubes
    DEBUG_MoveCube(&CubeMoving, DeltaTime, PlayerCubeVelocity);

    // 4. Render
    UseShader(Shader);
    SetUniformMat4F(Shader, "Projection", false, glm::value_ptr(Projection));
    SetUniformMat4F(Shader, "View", false, glm::value_ptr(View));

    DEBUG_RenderCube(Shader, &CubeStatic);
    DEBUG_RenderCube(Shader, &CubeMoving);

    *Out_PlayerCubePosition = CubeMoving.Position;
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
        glm::vec4 Transformed = Transform * glm::vec4(Vertices[Index], 1.0f);

        Out_TransformedVertices[Index].x = Transformed.x;
        Out_TransformedVertices[Index].y = Transformed.y;
        Out_TransformedVertices[Index].z = Transformed.z;
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
    Result.IsColliding = false;
    Result.VAO = CubeVAO;
    DEBUG_ProcessCubePosition(&Result);
    return Result;
}

void
DEBUG_MoveCube(cube *Cube, f32 DeltaTime, glm::vec3 Velocity)
{
    Cube->Position += Velocity * DeltaTime;

    DEBUG_ProcessCubePosition(Cube);

    DEBUG_CheckCollisions(Cube, &CubeStatic);

    if (Cube->IsColliding)
    {
        printf("Velocity: %.2f, %.2f, %.2f\n", Velocity.x, Velocity.y, Velocity.z);
        printf("Speculative position: %.2f, %.2f, %.2f\n", Cube->Position.x, Cube->Position.y, Cube->Position.z);
        printf("Position error: %.2f, %.2f, %.2f\n", Cube->PositionError.x, Cube->PositionError.y, Cube->PositionError.z);
        Cube->Position -= Cube->PositionError;
        printf("Fixed position: %.2f, %.2f, %.2f\n", Cube->Position.x, Cube->Position.y, Cube->Position.z);
        DEBUG_ProcessCubePosition(Cube);
        printf("Position error: %.2f, %.2f, %.2f\n", Cube->PositionError.x, Cube->PositionError.y, Cube->PositionError.z);
        Cube->IsColliding = false;
        CubeStatic.IsColliding = false;
    }
}

void
DEBUG_ProcessCubePosition(cube *Cube)
{
    Cube->PositionError = glm::vec3(0.0f);

    glm::mat4 Model(1.0f);
    Model = glm::translate(Model, Cube->Position);
    Model = glm::scale(Model, Cube->Scale); // TODO: Should scale be baked into VBO and original vertices already?
    Cube->TransientModelTransform = Model;
    DEBUG_TransformVertices(Model, Cube->Vertices, Cube->TransientTransformedVertices, 8);
}

void
DEBUG_CheckCollisions(cube *CubeA, cube *CubeB)
{
    glm::vec3 CubeAMin;
    glm::vec3 CubeAMax;
    glm::vec3 CubeACenter;
    glm::vec3 CubeBMin;
    glm::vec3 CubeBMax;
    glm::vec3 CubeBCenter;
    DEBUG_GetAABB(CubeA, &CubeAMin, &CubeAMax, &CubeACenter, NULL);
    DEBUG_GetAABB(CubeB, &CubeBMin, &CubeBMax, &CubeBCenter, NULL);

    bool IsColliding = true;
    for (i32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        IsColliding &= (CubeAMin[AxisIndex] < CubeBMax[AxisIndex] &&
                        CubeAMax[AxisIndex] > CubeBMin[AxisIndex]);
    }
    CubeA->IsColliding = IsColliding;
    CubeB->IsColliding = IsColliding;

    if (IsColliding)
    {
        glm::vec3 Direction = glm::normalize(CubeBCenter - CubeACenter);
        f32 GreatestAxisValue = 0.0f;
        i32 GreatestAxis = 0;
        for (i32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
        {
            if (fabs(Direction[AxisIndex]) > GreatestAxisValue)
            {
                GreatestAxisValue = fabs(Direction[AxisIndex]);
                GreatestAxis = AxisIndex;
            }
        }
        if (Direction[GreatestAxis] > 0.0f) // NOTE: assumes centers never coincide (wrong obv)
        {
            CubeA->PositionError[GreatestAxis] = CubeAMax[GreatestAxis] - CubeBMin[GreatestAxis];
        }
        else
        {
            CubeA->PositionError[GreatestAxis] = CubeAMin[GreatestAxis] - CubeBMax[GreatestAxis];
        }
        
        //for (i32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
        //{
        //    f32 AxisError = 0.0f;
        //    if (CubeACenter[AxisIndex] < CubeBCenter[AxisIndex])
        //    {
        //        AxisError = CubeAMax[AxisIndex] - CubeBMin[AxisIndex];
        //    }
        //    else if (CubeACenter[AxisIndex] > CubeBCenter[AxisIndex])
        //    {
        //        AxisError = CubeBMax[AxisIndex] - CubeAMin[AxisIndex];
        //    }

        //    CubeA->PositionError[AxisIndex] = AxisError;
        //}
    }
}

void
DEBUG_GetAABB(cube *Cube, glm::vec3 *Out_Min, glm::vec3 *Out_Max, glm::vec3 *Out_Center, glm::vec3 *Out_Extents)
{
    glm::vec3 Min(FLT_MAX, FLT_MAX, FLT_MAX);
    glm::vec3 Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    i32 VertexCount = 8;
    glm::vec3 *Vertices = Cube->TransientTransformedVertices;
    for (i32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
    {
        for (i32 Index = 0; Index < 3; ++Index)
        {
            if (Vertices[VertexIndex][Index] > Max[Index])
            {
                Max[Index] = Vertices[VertexIndex][Index];
            }
            else if (Vertices[VertexIndex][Index] < Min[Index])
            {
                Min[Index] = Vertices[VertexIndex][Index];
            }
        }
    }
    
    if (Out_Min)
    {
        *Out_Min = Min;
    }
    if (Out_Max)
    {
        *Out_Max = Max;
    }

    if (Out_Center || Out_Extents)
    {
        glm::vec3 Extents = (Max - Min) / 2.0f;
        glm::vec3 Center = Min + Extents;
        if (Out_Center)
        {
            *Out_Center = Center;
        }
        if (Out_Extents)
        {
            *Out_Extents = Extents;
        }
    }
}

void
DEBUG_RenderCube(u32 Shader, cube *Cube)
{
    SetUniformMat4F(Shader, "Model", false, glm::value_ptr(Cube->TransientModelTransform));

    if (!Cube->IsColliding)
    {
        SetUniformVec3F(Shader, "Color", false, &ShapeColorNoCollision[0]);
    }
    else
    {
        SetUniformVec3F(Shader, "Color", false, &ShapeColorCollision[0]);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(3);

    glBindVertexArray(Cube->VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

#endif