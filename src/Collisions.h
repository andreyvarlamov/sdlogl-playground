#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cfloat>
#include <cstdlib>

#include "Common.h"
#include "Math.h"
#include "Shader.h"
#include "Util.h"

#define DEBUG_POINT_BUFFER_SIZE 16
struct debug_points
{
    glm::vec3 Points[DEBUG_POINT_BUFFER_SIZE];
    glm::vec3 PointColors[DEBUG_POINT_BUFFER_SIZE];
    i32 PointCount;
    i32 PointBufferSize;

    u32 VAO;
};

struct cube
{
    glm::vec3 Vertices[8];
    glm::vec3 Scale;
    glm::vec3 Position;

    glm::mat4 TransientModelTransform;
    glm::vec3 TransientTransformedVertices[8];
    glm::vec3 PositionAdjustment;
    bool IsColliding;

    u32 VAO;
};

#define SPHERE_RING_COUNT 10
#define SPHERE_SECTOR_COUNT 15
struct sphere
{
    glm::vec3 Vertices[SPHERE_RING_COUNT * SPHERE_SECTOR_COUNT];
    i32 VertexCount;
    i32 IndexCount;
    glm::vec3 Position;
    f32 Radius;

    glm::mat4 TransientModelTransform;
    glm::vec3 TransientTransformedVertices[SPHERE_RING_COUNT * SPHERE_SECTOR_COUNT];
    bool IsColliding;

    u32 VAO;
};

glm::vec3 ShapeColorNoCollision(0.8f, 0.8f, 0.0f);
glm::vec3 ShapeColorCollision(1.0f, 0.7f, 0.7f);
glm::vec3 PointColorPink(1.0f, 0.0f, 1.0f);
glm::vec3 PointColorRed(1.0f, 0.0f, 0.0f);

debug_points DebugPointsStorage;

cube CubeStatic;

cube CubeMoving;
sphere SphereMoving;

#define SPHERE 1

debug_points
DEBUG_InitializeDebugPoints(i32 PointBufferSize);
cube
DEBUG_GenerateCube(glm::vec3 Position, glm::vec3 Scale);
void
DEBUG_GetUnitCubeVerticesAndIndices(glm::vec3 *Out_Vertices, i32 *Out_Indices);
sphere
DEBUG_GenerateSphere(glm::vec3 Position, f32 Radius);
void
DEBUG_GetUnitSphereVerticesAndIndices(glm::vec3 *Out_Vertices, i32 *Out_Indices, i32 RingCount, i32 SectorCount);
f32 *
DEBUG_GetRawVertexDataFromVec3(glm::vec3 *Vertex, i32 VertexCount);
void
DEBUG_ProcessCubePosition(cube *Cube);
void
DEBUG_ProcessSpherePosition(sphere *Sphere);
void
DEBUG_TransformVertices(glm::mat4 Transform, glm::vec3 *Vertices, glm::vec3 *Out_TransformedVertices, i32 VertexCount);
void
DEBUG_MoveCube(cube *Cube, f32 DeltaTime, glm::vec3 Velocity);
void
DEBUG_MoveSphere(sphere *Sphere, f32 DeltaTime, glm::vec3 Velocity);
void
DEBUG_GetAABB(cube *Cube, glm::vec3 *Out_Min, glm::vec3 *Out_Max, glm::vec3 *Out_Center, glm::vec3 *Out_Extents);
void
DEBUG_AddDebugPoint(debug_points *DebugPoints, glm::vec3 Position, glm::vec3 Color);
void
DEBUG_RenderCube(u32 Shader, cube *Cube);
void
DEBUG_RenderSphere(u32 Shader, sphere *Sphere);
void
DEBUG_RenderDebugPoints(u32 Shader, debug_points *DebugPoints);

void
DEBUG_CollisionTestSetup(u32 Shader)
{
    DebugPointsStorage = DEBUG_InitializeDebugPoints(DEBUG_POINT_BUFFER_SIZE);

    CubeStatic = DEBUG_GenerateCube(glm::vec3(4.0f, 1.0f, -2.0f), glm::vec3(2.0f, 1.0f, 1.0f));
    
#if CUBE
    CubeMoving = DEBUG_GenerateCube(glm::vec3(4.0f, 1.0f, 2.0f), glm::vec3(1.0f, 1.0f, 2.0f));
#elif SPHERE
    SphereMoving = DEBUG_GenerateSphere(glm::vec3(4.0f, 1.0f, 2.0f), 0.8f);
#endif
}

void
DEBUG_CollisionTestUpdate(u32 Shader, f32 DeltaTime,
                          glm::mat4 Projection, glm::mat4 View,
                          glm::vec3 PlayerShapeVelocity,
                          glm::vec3 *Out_PlayerShapePosition)
{
#if CUBE
    DEBUG_MoveCube(&CubeMoving, DeltaTime, PlayerShapeVelocity);
#elif SPHERE
    DEBUG_MoveSphere(&SphereMoving, DeltaTime, PlayerShapeVelocity);
#endif

    UseShader(Shader);
    SetUniformMat4F(Shader, "Projection", false, glm::value_ptr(Projection));
    SetUniformMat4F(Shader, "View", false, glm::value_ptr(View));

    DEBUG_RenderCube(Shader, &CubeStatic);
    DEBUG_AddDebugPoint(&DebugPointsStorage, CubeStatic.Position, PointColorPink);

#if CUBE
    DEBUG_RenderCube(Shader, &CubeMoving);
#elif SPHERE
    DEBUG_RenderSphere(Shader, &SphereMoving);
    DEBUG_AddDebugPoint(&DebugPointsStorage, SphereMoving.Position, PointColorPink);

#endif

#if CUBE
    *Out_PlayerShapePosition = CubeMoving.Position;
#elif SPHERE
    *Out_PlayerShapePosition = SphereMoving.Position;
#endif

    DEBUG_RenderDebugPoints(Shader, &DebugPointsStorage);
}

debug_points
DEBUG_InitializeDebugPoints(i32 PointBufferSize)
{
    f32 Point[] = { 0.0f, 0.0f, 0.0f };

    u32 VAO;
    glGenVertexArrays(1, &VAO);
    u32 VBO;
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 1 * 3 * sizeof(f32), Point, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (void *) 0);
    glBindVertexArray(0);

    debug_points Result {};
    Result.VAO = VAO;
    Result.PointBufferSize = PointBufferSize;
    return Result;
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(i32), CubeIndices, GL_STATIC_DRAW);
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

sphere
DEBUG_GenerateSphere(glm::vec3 Position, f32 Radius)
{
    glm::vec3 Vertices[SPHERE_RING_COUNT * SPHERE_SECTOR_COUNT];
    i32 Indices[SPHERE_RING_COUNT * SPHERE_SECTOR_COUNT * 6];
    i32 RingCount = SPHERE_RING_COUNT;
    i32 SectorCount = SPHERE_SECTOR_COUNT;
    DEBUG_GetUnitSphereVerticesAndIndices(Vertices, Indices, RingCount, SectorCount);

    i32 VertexCount = RingCount * SectorCount;
    i32 IndexCount = RingCount * SectorCount * 6;
    f32 *RawData = DEBUG_GetRawVertexDataFromVec3(Vertices, VertexCount);
    u32 VAO;
    glGenVertexArrays(1, &VAO);
    u32 VBO;
    glGenBuffers(1, &VBO);
    u32 EBO;
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, VertexCount * 3 * sizeof(f32), RawData, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexCount * sizeof(i32), Indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (void *) 0);
    glBindVertexArray(0);
    free(RawData);

    sphere Result;
    memcpy_s(Result.Vertices, VertexCount * sizeof(glm::vec3), Vertices, VertexCount * sizeof(glm::vec3));
    Result.VertexCount = VertexCount;
    Result.IndexCount = IndexCount;
    Result.Radius = Radius;
    Result.Position = Position;
    Result.VAO = VAO;
    DEBUG_ProcessSpherePosition(&Result);
    return Result;
}

void
DEBUG_GetUnitCubeVerticesAndIndices(glm::vec3 *Out_Vertices, i32 *Out_Indices)
{
    Assert(Out_Vertices);
    Assert(Out_Indices);
    Out_Vertices[0].x = -0.5f; Out_Vertices[0].y =  0.5f; Out_Vertices[0].z =  0.5f;
    Out_Vertices[1].x = -0.5f; Out_Vertices[1].y = -0.5f; Out_Vertices[1].z =  0.5f;
    Out_Vertices[2].x =  0.5f; Out_Vertices[2].y = -0.5f; Out_Vertices[2].z =  0.5f;
    Out_Vertices[3].x =  0.5f; Out_Vertices[3].y =  0.5f; Out_Vertices[3].z =  0.5f;
    Out_Vertices[4].x =  0.5f; Out_Vertices[4].y =  0.5f; Out_Vertices[4].z = -0.5f;
    Out_Vertices[5].x =  0.5f; Out_Vertices[5].y = -0.5f; Out_Vertices[5].z = -0.5f;
    Out_Vertices[6].x = -0.5f; Out_Vertices[6].y = -0.5f; Out_Vertices[6].z = -0.5f;
    Out_Vertices[7].x = -0.5f; Out_Vertices[7].y =  0.5f; Out_Vertices[7].z = -0.5f;

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

void
DEBUG_GetUnitSphereVerticesAndIndices(glm::vec3 *Out_Vertices, i32 *Out_Indices, i32 RingCount, i32 SectorCount)
{
    Assert(Out_Vertices);
    Assert(Out_Indices);

    f32 R = 1.0f / (f32) (RingCount - 1);
    f32 S = 1.0f / (f32) (SectorCount - 1);

    i32 VertexIndex = 0;
    i32 IndexIndex = 0;

    for (i32 RingIndex = 0; RingIndex < RingCount; ++RingIndex)
    {
        for (i32 SectorIndex = 0; SectorIndex < SectorCount; ++SectorIndex)
        {
            f32 X = cosf(2.0f * PI * SectorIndex * S) * sinf(PI * RingIndex * R);
            f32 Y = sinf(-PI / 2.0f + PI * RingIndex * R);
            f32 Z = sinf(2.0f * PI * SectorIndex * S) * sinf(PI * RingIndex * R);

            Out_Vertices[VertexIndex].x = X;
            Out_Vertices[VertexIndex].y = Y;
            Out_Vertices[VertexIndex].z = Z;

            VertexIndex++;
        }
    }

    for (i32 RingIndex = 0; RingIndex < RingCount; ++RingIndex)
    {
        for (i32 SectorIndex = 0; SectorIndex < SectorCount; ++SectorIndex)
        {
            Out_Indices[IndexIndex++] = RingIndex * SectorCount + SectorIndex;
            Out_Indices[IndexIndex++] = (RingIndex + 1) * SectorCount + SectorIndex;
            Out_Indices[IndexIndex++] = RingIndex * SectorCount + (SectorIndex + 1);
            Out_Indices[IndexIndex++] = RingIndex * SectorCount + (SectorIndex + 1);
            Out_Indices[IndexIndex++] = (RingIndex + 1) * SectorCount + SectorIndex;
            Out_Indices[IndexIndex++] = (RingIndex + 1) * SectorCount + (SectorIndex + 1);
        }
    }
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
DEBUG_ProcessCubePosition(cube *Cube)
{
    Cube->PositionAdjustment = glm::vec3(0.0f);

    glm::mat4 Model(1.0f);
    Model = glm::translate(Model, Cube->Position);
    Model = glm::scale(Model, Cube->Scale); // TODO: Should scale be baked into VBO and original vertices already?
    Cube->TransientModelTransform = Model;
    DEBUG_TransformVertices(Model, Cube->Vertices, Cube->TransientTransformedVertices, 8);
}

void
DEBUG_ProcessSpherePosition(sphere *Sphere)
{
    glm::mat4 Model(1.0f);
    Model = glm::translate(Model, Sphere->Position);
    Model = glm::scale(Model, glm::vec3(Sphere->Radius, Sphere->Radius, Sphere->Radius));
    Sphere->TransientModelTransform = Model;
    DEBUG_TransformVertices(Model, Sphere->Vertices, Sphere->TransientTransformedVertices, Sphere->VertexCount);
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

void
DEBUG_MoveCube(cube *Cube, f32 DeltaTime, glm::vec3 Velocity)
{
    Cube->Position += Velocity * DeltaTime;

    DEBUG_ProcessCubePosition(Cube);

    glm::vec3 MovingCubeMin;
    glm::vec3 MovingCubeMax;
    glm::vec3 MovingCubeCenter;
    glm::vec3 StaticCubeMin;
    glm::vec3 StaticCubeMax;
    glm::vec3 StaticCubeCenter;
    glm::vec3 StaticCubeExtents;
    DEBUG_GetAABB(Cube, &MovingCubeMin, &MovingCubeMax, &MovingCubeCenter, NULL);
    DEBUG_GetAABB(&CubeStatic, &StaticCubeMin, &StaticCubeMax, &StaticCubeCenter, &StaticCubeExtents);

    bool IsColliding = true;
    for (i32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        IsColliding &= (MovingCubeMin[AxisIndex] < StaticCubeMax[AxisIndex] &&
                        MovingCubeMax[AxisIndex] > StaticCubeMin[AxisIndex]);
    }
    Cube->IsColliding = IsColliding;
    CubeStatic.IsColliding = IsColliding;

    if (IsColliding)
    {
        printf("MovingCubeMin: %.4f, %.4f, %.4f; MovingCubeMax: %.4f, %.4f, %.4f\n",
               MovingCubeMin.x, MovingCubeMin.y, MovingCubeMin.z,
               MovingCubeMax.x, MovingCubeMax.y, MovingCubeMax.z);
        printf("StaticCubeMin: %.4f, %.4f, %.4f; StaticCubeMax: %.4f, %.4f, %.4f\n",
               StaticCubeMin.x, StaticCubeMin.y, StaticCubeMin.z,
               StaticCubeMax.x, StaticCubeMax.y, StaticCubeMax.z);
        f32 MinPenetration = FLT_MAX;
        i32 MinPenetrationAxis = 0;
        bool MinPenetrationFromBMaxSide = false;
        for (i32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
        {
            f32 PenetrationFromBMaxSide = StaticCubeMax[AxisIndex] - MovingCubeMin[AxisIndex];
            Assert(PenetrationFromBMaxSide >= 0.0f);
            if (PenetrationFromBMaxSide < MinPenetration)
            {
                MinPenetration = PenetrationFromBMaxSide;
                MinPenetrationAxis = AxisIndex;
                MinPenetrationFromBMaxSide = true;
            }

            f32 PenetrationFromBMinSide = MovingCubeMax[AxisIndex] - StaticCubeMin[AxisIndex];
            Assert(PenetrationFromBMinSide >= 0.0f);
            if (PenetrationFromBMinSide < MinPenetration)
            {
                MinPenetration = PenetrationFromBMinSide;
                MinPenetrationAxis = AxisIndex;
                MinPenetrationFromBMaxSide = false;
            }
        }

        f32 Sign;
        if (MinPenetrationFromBMaxSide)
        {
            Sign = 1.0f;
        }
        else
        {
            Sign = -1.0f;
        }

        Cube->PositionAdjustment[MinPenetrationAxis] = Sign * MinPenetration;
    }

    if (Cube->IsColliding)
    {
        printf("Velocity: %.2f, %.2f, %.2f\n", Velocity.x, Velocity.y, Velocity.z);
        printf("Speculative position: %.2f, %.2f, %.2f\n", Cube->Position.x, Cube->Position.y, Cube->Position.z);
        printf("Position adjustment: %.2f, %.2f, %.2f\n", Cube->PositionAdjustment.x, Cube->PositionAdjustment.y, Cube->PositionAdjustment.z);
        Cube->Position += Cube->PositionAdjustment;
        printf("Fixed position: %.2f, %.2f, %.2f\n", Cube->Position.x, Cube->Position.y, Cube->Position.z);
        DEBUG_ProcessCubePosition(Cube);
        printf("Position adjustment: %.2f, %.2f, %.2f\n", Cube->PositionAdjustment.x, Cube->PositionAdjustment.y, Cube->PositionAdjustment.z);
        Cube->IsColliding = false;
        CubeStatic.IsColliding = false;
    }
}

void
DEBUG_MoveSphere(sphere *Sphere, f32 DeltaTime, glm::vec3 Velocity)
{
    Sphere->Position += Velocity * DeltaTime;

    DEBUG_ProcessSpherePosition(Sphere);

    glm::vec3 StaticCubeMin;
    glm::vec3 StaticCubeMax;
    glm::vec3 StaticCubeCenter;
    glm::vec3 StaticCubeExtents;
    DEBUG_GetAABB(&CubeStatic, &StaticCubeMin, &StaticCubeMax, &StaticCubeCenter, &StaticCubeExtents);

    bool IsColliding = false;
    glm::vec3 VectorBetweenCenters = Sphere->Position - StaticCubeCenter;
    glm::vec3 NearestPointOnStaticCube = VectorBetweenCenters;
    for (i32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        if (NearestPointOnStaticCube[AxisIndex] > StaticCubeExtents[AxisIndex])
        {
            NearestPointOnStaticCube[AxisIndex] = StaticCubeExtents[AxisIndex];
        }
        else if (NearestPointOnStaticCube[AxisIndex] < -StaticCubeExtents[AxisIndex])
        {
            NearestPointOnStaticCube[AxisIndex] = -StaticCubeExtents[AxisIndex];
        }
    }
    glm::vec3 NearestPointGlobal = NearestPointOnStaticCube + StaticCubeCenter;
    glm::vec3 VectorBetweenSphereCenterAndNearestPoint = NearestPointGlobal - Sphere->Position;
    f32 LengthSqr = GetSquaredVectorLength(VectorBetweenSphereCenterAndNearestPoint);
    f32 RadiusSqr = Sphere->Radius * Sphere->Radius;
    Sphere->IsColliding = LengthSqr < RadiusSqr;
    CubeStatic.IsColliding = LengthSqr < RadiusSqr;

    DEBUG_AddDebugPoint(&DebugPointsStorage, NearestPointGlobal, PointColorRed);
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
DEBUG_AddDebugPoint(debug_points *DebugPoints, glm::vec3 Position, glm::vec3 Color)
{
    Assert(DebugPoints->PointCount < DebugPoints->PointBufferSize - 1);

    DebugPoints->Points[DebugPoints->PointCount] = Position;
    DebugPoints->PointColors[DebugPoints->PointCount] = Color;

    DebugPoints->PointCount++;
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

void
DEBUG_RenderSphere(u32 Shader, sphere *Sphere)
{
    SetUniformMat4F(Shader, "Model", false, glm::value_ptr(Sphere->TransientModelTransform));
    
    if (!Sphere->IsColliding)
    {
        SetUniformVec3F(Shader, "Color", false, &ShapeColorNoCollision[0]);
    }
    else
    {
        SetUniformVec3F(Shader, "Color", false, &ShapeColorCollision[0]);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(3);

    glBindVertexArray(Sphere->VAO);
    glDrawElements(GL_TRIANGLES, Sphere->IndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void
DEBUG_RenderDebugPoints(u32 Shader, debug_points *DebugPoints)
{
    for (i32 PointIndex = 0; PointIndex < DebugPoints->PointCount; ++PointIndex)
    {
        glm::mat4 Model(1.0f);
        Model = glm::translate(Model, DebugPoints->Points[PointIndex]);
        SetUniformMat4F(Shader, "Model", false, glm::value_ptr(Model));

        SetUniformVec3F(Shader, "Color", false, &DebugPoints->PointColors[PointIndex][0]);

        glPointSize(10);

        glBindVertexArray(DebugPoints->VAO);
        glDrawArrays(GL_POINTS, 0, 1);
        glBindVertexArray(0);
    }

    DebugPoints->PointCount = 0;
}

#endif