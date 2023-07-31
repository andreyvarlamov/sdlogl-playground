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

#define DEBUG_VECTOR_BUFFER_SIZE 16
struct debug_vectors
{
    glm::vec3 StartPoints[DEBUG_VECTOR_BUFFER_SIZE];
    glm::vec3 EndPoints[DEBUG_VECTOR_BUFFER_SIZE];
    glm::vec3 VectorColors[DEBUG_VECTOR_BUFFER_SIZE];
    i32 VectorCount;
    i32 VectorBufferSize;

    u32 VBO;
    u32 VAO;
};

struct box
{
    glm::vec3 Vertices[8];
    glm::vec3 Scale;
    glm::vec3 Position;

    glm::mat4 TransientModelTransform;
    glm::vec3 TransientTransformedVertices[8];
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

glm::vec3 ColorRed(1.0f, 0.0f, 0.0f);
glm::vec3 ColorBlue(0.0f, 0.0f, 1.0f);
glm::vec3 ColorGreen(0.0f, 1.0f, 0.0f);

glm::vec3 ColorYellow(0.8f, 0.8f, 0.0f);
glm::vec3 ColorOrange(1.0f, 0.6f, 0.3f);
glm::vec3 ColorPink(1.0f, 0.0f, 1.0f);

glm::vec3 ColorWhite(1.0f, 1.0f, 1.0f);
glm::vec3 ColorGrey(0.2f, 0.2f, 0.2f);

debug_points DebugPointsStorage;
debug_vectors DebugVectorsStorage;

#define RESOLVE_COLLISIONS 1

#define BOX_BOX 1
#define SPHERE_BOX 0
#define BOX_SPHERE 0

#if BOX_BOX
box BoxMoving;
box BoxStatic;
#elif SPHERE_BOX
sphere SphereMoving;
box BoxStatic;
#elif BOX_SPHERE
box BoxMoving;
sphere SphereStatic;
#endif

debug_points
DEBUG_InitializeDebugPoints(i32 PointBufferSize);
debug_vectors
DEBUG_InitializeDebugVectors(i32 VectorBufferSize);
box
DEBUG_GenerateBox(glm::vec3 Position, glm::vec3 Scale);
void
DEBUG_GetUnitBoxVerticesAndIndices(glm::vec3 *Out_Vertices, i32 *Out_Indices);
sphere
DEBUG_GenerateSphere(glm::vec3 Position, f32 Radius);
void
DEBUG_GetUnitSphereVerticesAndIndices(glm::vec3 *Out_Vertices, i32 *Out_Indices, i32 RingCount, i32 SectorCount);
f32 *
DEBUG_GetRawVertexDataFromVec3(glm::vec3 *Vertex, i32 VertexCount);
void
DEBUG_ProcessBoxPosition(box *Box);
void
DEBUG_ProcessSpherePosition(sphere *Sphere);
void
DEBUG_TransformVertices(glm::mat4 Transform, glm::vec3 *Vertices, glm::vec3 *Out_TransformedVertices, i32 VertexCount);
void
DEBUG_MoveBox(box *Box, f32 DeltaTime, glm::vec3 Velocity);
void
DEBUG_MoveSphere(sphere *Sphere, f32 DeltaTime, glm::vec3 Velocity);
void
DEBUG_GetAABB(box *Box, glm::vec3 *Out_Min, glm::vec3 *Out_Max, glm::vec3 *Out_Center, glm::vec3 *Out_Extents);
void
DEBUG_AddDebugPoint(debug_points *DebugPoints, glm::vec3 Position, glm::vec3 Color);
void
DEBUG_AddDebugVector(debug_vectors *DebugVectors, glm::vec3 VectorStart, glm::vec3 VectorEnd, glm::vec3 Color);
void
DEBUG_RenderBox(u32 Shader, box *Box);
void
DEBUG_RenderSphere(u32 Shader, sphere *Sphere);
void
DEBUG_RenderDebugPoints(u32 Shader, debug_points *DebugPoints);
void
DEBUG_RenderDebugVectors(u32 Shader, debug_vectors *DebugVectors);

void
DEBUG_CollisionTestSetup(u32 Shader)
{
    DebugPointsStorage = DEBUG_InitializeDebugPoints(DEBUG_POINT_BUFFER_SIZE);
    DebugVectorsStorage = DEBUG_InitializeDebugVectors(DEBUG_VECTOR_BUFFER_SIZE);

#if BOX_BOX
    BoxMoving = DEBUG_GenerateBox(glm::vec3(4.0f, 1.0f, 2.0f), glm::vec3(1.0f, 1.0f, 2.0f));
    BoxStatic = DEBUG_GenerateBox(glm::vec3(4.0f, 1.0f, -2.0f), glm::vec3(2.0f, 1.0f, 1.0f));
#elif SPHERE_BOX
    SphereMoving = DEBUG_GenerateSphere(glm::vec3(4.0f, 1.0f, 2.0f), 0.5f);
    BoxStatic = DEBUG_GenerateBox(glm::vec3(4.0f, 1.0f, -2.0f), glm::vec3(2.0f, 1.0f, 1.0f));
#elif BOX_SPHERE
    BoxMoving = DEBUG_GenerateBox(glm::vec3(4.0f, 1.0f, 2.0f), glm::vec3(1.0f, 1.0f, 2.0f));
    SphereStatic = DEBUG_GenerateSphere(glm::vec3(4.0f, 1.0f, -2.0f), 0.5f);
#endif
}

void
DEBUG_CollisionTestUpdate(u32 DebugCollisionShader, u32 DebugDrawShader,
                          f32 DeltaTime,
                          glm::mat4 Projection, glm::mat4 View,
                          glm::vec3 PlayerShapeVelocity,
                          glm::vec3 *Out_PlayerShapePosition)
{
#if BOX_BOX
    DEBUG_MoveBox(&BoxMoving, DeltaTime, PlayerShapeVelocity);

    UseShader(DebugCollisionShader);
    SetUniformMat4F(DebugCollisionShader, "Projection", false, glm::value_ptr(Projection));
    SetUniformMat4F(DebugCollisionShader, "View", false, glm::value_ptr(View));

    DEBUG_RenderBox(DebugCollisionShader, &BoxMoving);
    DEBUG_AddDebugPoint(&DebugPointsStorage, BoxMoving.Position, ColorPink);
    *Out_PlayerShapePosition = BoxMoving.Position;
    DEBUG_RenderBox(DebugCollisionShader, &BoxStatic);
    DEBUG_AddDebugPoint(&DebugPointsStorage, BoxStatic.Position, ColorPink);
#elif SPHERE_BOX
    DEBUG_MoveSphere(&SphereMoving, DeltaTime, PlayerShapeVelocity);

    UseShader(DebugCollisionShader);
    SetUniformMat4F(DebugCollisionShader, "Projection", false, glm::value_ptr(Projection));
    SetUniformMat4F(DebugCollisionShader, "View", false, glm::value_ptr(View));

    DEBUG_RenderSphere(DebugCollisionShader, &SphereMoving);
    DEBUG_AddDebugPoint(&DebugPointsStorage, SphereMoving.Position, ColorPink);
    *Out_PlayerShapePosition = SphereMoving.Position;
    DEBUG_RenderBox(DebugCollisionShader, &BoxStatic);
    DEBUG_AddDebugPoint(&DebugPointsStorage, BoxStatic.Position, ColorPink);
#elif BOX_SPHERE
#endif

    DEBUG_RenderDebugPoints(DebugCollisionShader, &DebugPointsStorage);
    UseShader(DebugDrawShader);
    SetUniformMat4F(DebugDrawShader, "Projection", false, glm::value_ptr(Projection));
    SetUniformMat4F(DebugDrawShader, "View", false, glm::value_ptr(View));
    DEBUG_RenderDebugVectors(DebugDrawShader, &DebugVectorsStorage);
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

    debug_points Result{};
    Result.VAO = VAO;
    Result.PointBufferSize = PointBufferSize;
    return Result;
}

debug_vectors
DEBUG_InitializeDebugVectors(i32 VectorBufferSize)
{
    u32 VAO;
    glGenVertexArrays(1, &VAO);
    u32 VBO;
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, (2 + 2) * 3 * sizeof(f32), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, (2 + 1) * 3 * sizeof(f32), 3 * sizeof(f32), &ColorWhite);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (void *) 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (void *) (2 * 3 * sizeof(f32)));
    glBindVertexArray(0);

    debug_vectors Result{};
    Result.VAO = VAO;
    Result.VBO = VBO;
    Result.VectorBufferSize = VectorBufferSize;
    return Result;
}

box
DEBUG_GenerateBox(glm::vec3 Position, glm::vec3 Scale)
{
    glm::vec3 Vertices[8];
    i32 Indices[36];
    DEBUG_GetUnitBoxVerticesAndIndices(Vertices, Indices);

    f32 *RawData = DEBUG_GetRawVertexDataFromVec3(Vertices, 8);
    u32 VAO;
    glGenVertexArrays(1, &VAO);
    u32 VBO;
    glGenBuffers(1, &VBO);
    u32 EBO;
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 8 * 3 * sizeof(f32), RawData, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(i32), Indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (void *) 0);
    glBindVertexArray(0);
    free(RawData);

    box Result;
    memcpy_s(Result.Vertices, 8 * sizeof(glm::vec3), Vertices, 8 * sizeof(glm::vec3));
    Result.Scale = Scale;
    Result.Position = Position;
    Result.IsColliding = false;
    Result.VAO = VAO;
    DEBUG_ProcessBoxPosition(&Result);
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
DEBUG_GetUnitBoxVerticesAndIndices(glm::vec3 *Out_Vertices, i32 *Out_Indices)
{
    Assert(Out_Vertices);
    Assert(Out_Indices);
    Out_Vertices[0].x = -0.5f; Out_Vertices[0].y = 0.5f; Out_Vertices[0].z = 0.5f;
    Out_Vertices[1].x = -0.5f; Out_Vertices[1].y = -0.5f; Out_Vertices[1].z = 0.5f;
    Out_Vertices[2].x = 0.5f; Out_Vertices[2].y = -0.5f; Out_Vertices[2].z = 0.5f;
    Out_Vertices[3].x = 0.5f; Out_Vertices[3].y = 0.5f; Out_Vertices[3].z = 0.5f;
    Out_Vertices[4].x = 0.5f; Out_Vertices[4].y = 0.5f; Out_Vertices[4].z = -0.5f;
    Out_Vertices[5].x = 0.5f; Out_Vertices[5].y = -0.5f; Out_Vertices[5].z = -0.5f;
    Out_Vertices[6].x = -0.5f; Out_Vertices[6].y = -0.5f; Out_Vertices[6].z = -0.5f;
    Out_Vertices[7].x = -0.5f; Out_Vertices[7].y = 0.5f; Out_Vertices[7].z = -0.5f;

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
DEBUG_ProcessBoxPosition(box *Box)
{
    glm::mat4 Model(1.0f);
    Model = glm::translate(Model, Box->Position);
    Model = glm::scale(Model, Box->Scale); // TODO: Should scale be baked into VBO and original vertices already?
    Box->TransientModelTransform = Model;
    DEBUG_TransformVertices(Model, Box->Vertices, Box->TransientTransformedVertices, 8);
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
DEBUG_MoveBox(box *Box, f32 DeltaTime, glm::vec3 Velocity)
{
    Box->Position += Velocity * DeltaTime;

    DEBUG_ProcessBoxPosition(Box);

    glm::vec3 pgMovingBoxMin;
    glm::vec3 pgMovingBoxMax;
    glm::vec3 pgMovingBoxCenter;
    glm::vec3 pgStaticBoxMin;
    glm::vec3 pgStaticBoxMax;
    glm::vec3 pgStaticBoxCenter;
    glm::vec3 plStaticBoxMax;
    DEBUG_GetAABB(Box, &pgMovingBoxMin, &pgMovingBoxMax, &pgMovingBoxCenter, NULL);
    DEBUG_GetAABB(&BoxStatic, &pgStaticBoxMin, &pgStaticBoxMax, &pgStaticBoxCenter, &plStaticBoxMax);

    bool IsColliding = true;
    for (i32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        IsColliding &= (pgMovingBoxMin[AxisIndex] < pgStaticBoxMax[AxisIndex] &&
                        pgMovingBoxMax[AxisIndex] > pgStaticBoxMin[AxisIndex]);
    }
    Box->IsColliding = IsColliding;
    BoxStatic.IsColliding = IsColliding;

    DEBUG_AddDebugVector(&DebugVectorsStorage, pgStaticBoxCenter, pgMovingBoxCenter, ColorGrey);

    if (IsColliding)
    {
        f32 MinPenetration = FLT_MAX;
        i32 MinPenetrationAxis = 0;
        bool MinPenetrationFromBMaxSide = false;
        for (i32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
        {
            f32 PenetrationFromBMaxSide = pgStaticBoxMax[AxisIndex] - pgMovingBoxMin[AxisIndex];
            Assert(PenetrationFromBMaxSide >= 0.0f);
            if (PenetrationFromBMaxSide < MinPenetration)
            {
                MinPenetration = PenetrationFromBMaxSide;
                MinPenetrationAxis = AxisIndex;
                MinPenetrationFromBMaxSide = true;
            }

            f32 PenetrationFromBMinSide = pgMovingBoxMax[AxisIndex] - pgStaticBoxMin[AxisIndex];
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

        glm::vec3 PositionAdjustment{};
        PositionAdjustment[MinPenetrationAxis] = Sign * MinPenetration;

#if RESOLVE_COLLISIONS
        DEBUG_AddDebugVector(&DebugVectorsStorage, Box->Position, Box->Position + 0.3f * glm::normalize(PositionAdjustment), ColorBlue);
        Box->Position += PositionAdjustment;
        DEBUG_ProcessBoxPosition(Box);
        Box->IsColliding = false;
        BoxStatic.IsColliding = false;
#else
        DEBUG_AddDebugVector(&DebugVectorsStorage, Box->Position, Box->Position + PositionAdjustment, ColorBlue);
#endif
    }
}

void
DEBUG_MoveSphere(sphere *Sphere, f32 DeltaTime, glm::vec3 Velocity)
{
    Sphere->Position += Velocity * DeltaTime;

    DEBUG_ProcessSpherePosition(Sphere);

    glm::vec3 pgBoxMin;
    glm::vec3 pgBoxMax;
    glm::vec3 pgBoxCenter;
    glm::vec3 plBoxMax;
    DEBUG_GetAABB(&BoxStatic, &pgBoxMin, &pgBoxMax, &pgBoxCenter, &plBoxMax);

    bool IsColliding = false;
    glm::vec3 vBoxCenterSphereCenter = Sphere->Position - pgBoxCenter;
    glm::vec3 plClosestOnBox = vBoxCenterSphereCenter;
    for (i32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        if (plClosestOnBox[AxisIndex] > plBoxMax[AxisIndex])
        {
            plClosestOnBox[AxisIndex] = plBoxMax[AxisIndex];
        }
        else if (plClosestOnBox[AxisIndex] < -plBoxMax[AxisIndex])
        {
            plClosestOnBox[AxisIndex] = -plBoxMax[AxisIndex];
        }
    }
    glm::vec3 pgClosestOnBox = plClosestOnBox + pgBoxCenter;
    glm::vec3 vSphereCenterClosestOnBox = pgClosestOnBox - Sphere->Position;
    f32 lsvSphereCenterClosestOnBox = GetSquaredVectorLength(vSphereCenterClosestOnBox);
    f32 RadiusSqr = Sphere->Radius * Sphere->Radius;
    IsColliding = lsvSphereCenterClosestOnBox < RadiusSqr;
    Sphere->IsColliding = lsvSphereCenterClosestOnBox < RadiusSqr;
    BoxStatic.IsColliding = lsvSphereCenterClosestOnBox < RadiusSqr;

    if (IsColliding)
    {
        glm::vec3 plClosestOnSphere = glm::normalize(vSphereCenterClosestOnBox) * Sphere->Radius;
        glm::vec3 pgClosestOnSphere = plClosestOnSphere + Sphere->Position;
        glm::vec3 PositionAdjustment = pgClosestOnBox - pgClosestOnSphere;

#if RESOLVE_COLLISIONS
        DEBUG_AddDebugVector(&DebugVectorsStorage, Sphere->Position, Sphere->Position + 0.3f * glm::normalize(PositionAdjustment), ColorBlue);
        Sphere->Position += PositionAdjustment;
        DEBUG_ProcessSpherePosition(Sphere);
        Sphere->IsColliding = false;
        BoxStatic.IsColliding = false;
#else
        DEBUG_AddDebugVector(&DebugVectorsStorage, Sphere->Position, Sphere->Position + PositionAdjustment, ColorBlue);
        DEBUG_AddDebugPoint(&DebugPointsStorage, pgClosestOnSphere, ColorRed);
        DEBUG_AddDebugPoint(&DebugPointsStorage, pgClosestOnBox, ColorRed);
#endif
    }
}

void
DEBUG_GetAABB(box *Box, glm::vec3 *Out_Min, glm::vec3 *Out_Max, glm::vec3 *Out_Center, glm::vec3 *Out_Extents)
{
    glm::vec3 Min(FLT_MAX, FLT_MAX, FLT_MAX);
    glm::vec3 Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    i32 VertexCount = 8;
    glm::vec3 *Vertices = Box->TransientTransformedVertices;
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
DEBUG_AddDebugVector(debug_vectors *DebugVectors, glm::vec3 VectorStart, glm::vec3 VectorEnd, glm::vec3 Color)
{
    Assert(DebugVectors->VectorCount < DebugVectors->VectorBufferSize - 1);

    DebugVectors->StartPoints[DebugVectors->VectorCount] = VectorStart;
    DebugVectors->EndPoints[DebugVectors->VectorCount] = VectorEnd;
    DebugVectors->VectorColors[DebugVectors->VectorCount] = Color;

    DebugVectors->VectorCount++;
}

void
DEBUG_RenderBox(u32 Shader, box *Box)
{
    SetUniformMat4F(Shader, "Model", false, glm::value_ptr(Box->TransientModelTransform));

    if (!Box->IsColliding)
    {
        SetUniformVec3F(Shader, "Color", false, &ColorYellow[0]);
    }
    else
    {
        SetUniformVec3F(Shader, "Color", false, &ColorOrange[0]);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(3);

    glBindVertexArray(Box->VAO);
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
        SetUniformVec3F(Shader, "Color", false, &ColorYellow[0]);
    }
    else
    {
        SetUniformVec3F(Shader, "Color", false, &ColorOrange[0]);
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
    glBindVertexArray(DebugPoints->VAO);
    glPointSize(10);

    for (i32 PointIndex = 0; PointIndex < DebugPoints->PointCount; ++PointIndex)
    {
        glm::mat4 Model(1.0f);
        Model = glm::translate(Model, DebugPoints->Points[PointIndex]);
        SetUniformMat4F(Shader, "Model", false, glm::value_ptr(Model));

        SetUniformVec3F(Shader, "Color", false, &DebugPoints->PointColors[PointIndex][0]);

        glDrawArrays(GL_POINTS, 0, 1);
    }

    glBindVertexArray(0);

    DebugPoints->PointCount = 0;
}

bool AlreadyPrinted = false;

void
DEBUG_RenderDebugVectors(u32 Shader, debug_vectors *DebugVectors)
{
    glBindVertexArray(DebugVectors->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, DebugVectors->VBO);
    glLineWidth(5);

    for (i32 VectorIndex = 0; VectorIndex < DebugVectors->VectorCount; ++VectorIndex)
    {
        glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * sizeof(f32), &DebugVectors->StartPoints[VectorIndex][0]);
        glBufferSubData(GL_ARRAY_BUFFER, 3 * sizeof(f32), 3 * sizeof(f32), &DebugVectors->EndPoints[VectorIndex][0]);
        glBufferSubData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(f32), 3 * sizeof(f32), &DebugVectors->VectorColors[VectorIndex][0]);

        glDrawArrays(GL_LINES, 0, 2);

        if (!AlreadyPrinted)
        {
            PrintVBODataF(DebugVectors->VBO, 12);
            AlreadyPrinted = true;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    DebugVectors->VectorCount = 0;
}

#endif