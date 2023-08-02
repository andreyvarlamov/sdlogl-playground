#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cfloat>
#include <cstdio>
#include <cstring>

#include "Common.h"
#include "DebugUI.h"
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

struct aabb
{
    glm::vec3 Position;
    glm::vec3 Extents;

    bool IsColliding;
    i32 IndexCount;
    u32 VAO;
};

#define SPHERE_RING_COUNT 10
#define SPHERE_SECTOR_COUNT 15
struct sphere
{
    glm::vec3 Position;
    f32 Radius;

    bool IsColliding;
    i32 IndexCount;
    u32 VAO;
};

struct plane
{
    glm::vec3 Normal;
    f32 Distance;

    i32 IndexCount;
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

bool gResolveCollisions = false;

#define SHAPE_TYPE_COUNT 3
#define SHAPE_MAX_COUNT 4
aabb gBoxes[SHAPE_MAX_COUNT];
i32 gBoxCount = 0;
sphere gSpheres[SHAPE_MAX_COUNT];
i32 gSphereCount = 0;
plane gPlanes[SHAPE_MAX_COUNT];
i32 gPlaneCount = 0;
i32 gControlShapeType = 0;
i32 gControlShape = 0;

debug_points
DEBUG_InitializeDebugPoints(i32 PointBufferSize);
debug_vectors
DEBUG_InitializeDebugVectors(i32 VectorBufferSize);
aabb
DEBUG_GenerateBox(glm::vec3 Position, glm::vec3 Extents);
void
DEBUG_GetBoxVerticesAndIndices(glm::vec3 Extents, glm::vec3 *Out_Vertices, i32 *Out_Indices);
sphere
DEBUG_GenerateSphere(glm::vec3 Position, f32 Radius);
void
DEBUG_GetSphereVerticesAndIndices(f32 Radius, i32 RingCount, i32 SectorCount, glm::vec3 *Out_Vertices, i32 *Out_Indices);
plane
DEBUG_GeneratePlane(glm::vec3 Normal, f32 Distance);
void
DEBUG_GetPlaneVerticesAndIndices(glm::vec3 *Out_Vertices, i32 *Out_Indices);
f32 *
DEBUG_GetRawVertexDataFromVec3(glm::vec3 *Vertex, i32 VertexCount);
void
DEBUG_MoveBox(aabb *Box, f32 DeltaTime, glm::vec3 Velocity, bool ForceResolve);
void
DEBUG_MoveSphere(sphere *Sphere, f32 DeltaTime, glm::vec3 Velocity, bool ForceResolve);
bool
DEBUG_CheckCollision2AABB(aabb A, aabb B, glm::vec3 *Out_PositionAdjustment);
bool
DEBUG_CheckCollisionSphereAABB(sphere A, aabb B, glm::vec3 *Out_PositionAdjustment);
bool
DEBUG_CheckCollision2Sphere(sphere A, sphere B, glm::vec3 *Out_PositionAdjustment);
bool
DEBUG_ResolveCollision(glm::vec3 *Position, glm::vec3 Adjustment, bool ForceResolve);
bool
DEBUG_GetClosestPointOnBoxSurface(aabb Box, glm::vec3 *PointRelativeToCenter);
void
DEBUG_AddDebugPoint(debug_points *DebugPoints, glm::vec3 Position, glm::vec3 Color);
void
DEBUG_AddDebugVector(debug_vectors *DebugVectors, glm::vec3 VectorStart, glm::vec3 VectorEnd, glm::vec3 Color);
void
DEBUG_RenderBox(u32 Shader, aabb *Box, glm::vec3 Color);
void
DEBUG_RenderSphere(u32 Shader, sphere *Sphere, glm::vec3 Color);
void
DEBUG_RenderPlane(u32 Shader, plane *Plane, glm::vec3 Color);
void
DEBUG_RenderDebugPoints(u32 Shader, debug_points *DebugPoints);
void
DEBUG_RenderDebugVectors(u32 Shader, debug_vectors *DebugVectors);

void
DEBUG_CollisionTestSetup(u32 Shader)
{
    DebugPointsStorage = DEBUG_InitializeDebugPoints(DEBUG_POINT_BUFFER_SIZE);
    DebugVectorsStorage = DEBUG_InitializeDebugVectors(DEBUG_VECTOR_BUFFER_SIZE);

    gBoxes[gBoxCount++] = DEBUG_GenerateBox(glm::vec3(4.0f, 1.0f, 4.0f), glm::vec3(0.5f, 0.5f, 1.0f));
    gBoxes[gBoxCount++] = DEBUG_GenerateBox(glm::vec3(4.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.5f, 0.5f));
    
    gSpheres[gSphereCount++] = DEBUG_GenerateSphere(glm::vec3(3.5f, 1.0f, -2.0f), 0.8f);
    gSpheres[gSphereCount++] = DEBUG_GenerateSphere(glm::vec3(4.5f, 1.0f, -2.0f), 0.3f);
    
    gPlanes[gPlaneCount++] = DEBUG_GeneratePlane(glm::vec3(0.0f, 1.0f, 0.0f), 1.0f);

    Assert(gBoxCount <= SHAPE_MAX_COUNT);
    Assert(gSphereCount <= SHAPE_MAX_COUNT);
    Assert(gPlaneCount <= SHAPE_MAX_COUNT);
}

void
DEBUG_CollisionTestUpdate(u32 DebugCollisionShader, u32 DebugDrawShader,
                          f32 DeltaTime,
                          glm::mat4 Projection, glm::mat4 View,
                          glm::vec3 PlayerShapeVelocity,
                          bool ForceResolve, bool IncreaseControlShapeType, bool IncreaseControlShape)
{
    if (IncreaseControlShapeType)
    {
        if (++gControlShapeType >= SHAPE_TYPE_COUNT)
        {
            gControlShapeType = 0;
        }

        gControlShape = 0;
    }

    if (IncreaseControlShape)
    {
        i32 MaxShapeCount = gControlShapeType == 0 ? gBoxCount : gSphereCount;
        if (++gControlShape >= MaxShapeCount)
        {
            gControlShape = 0;
        }
    }

    char DebugBuffer[128];
    sprintf_s(DebugBuffer, "Control: {Type = %d, Shape = %d}", gControlShapeType, gControlShape);
    DEBUG_AddDebugString(DebugBuffer);

    if (gControlShapeType == 0)
    {
        if (gControlShape < gBoxCount)
        {
            aabb *MovingBox = &gBoxes[gControlShape];
            DEBUG_MoveBox(MovingBox, DeltaTime, PlayerShapeVelocity, ForceResolve);
        }
    }
    else if (gControlShapeType == 1)
    {
        if (gControlShape < gSphereCount)
        {
            sphere *MovingSphere = &gSpheres[gControlShape];
            DEBUG_MoveSphere(MovingSphere, DeltaTime, PlayerShapeVelocity, ForceResolve);
        }
    }

    UseShader(DebugCollisionShader);
    SetUniformMat4F(DebugCollisionShader, "Projection", false, glm::value_ptr(Projection));
    SetUniformMat4F(DebugCollisionShader, "View", false, glm::value_ptr(View));

    for (i32 BoxIndex = 0; BoxIndex < gBoxCount; ++BoxIndex)
    {
        aabb *Box = &gBoxes[BoxIndex];
        glm::vec3 Color = ColorGrey;
        if (gControlShapeType == 0 && gControlShape == BoxIndex)
        {
            if (!Box->IsColliding)
            {
                Color = ColorYellow;
            }
            else
            {
                Color = ColorOrange;
            }
        }

        DEBUG_RenderBox(DebugCollisionShader, Box, Color);
        DEBUG_AddDebugPoint(&DebugPointsStorage, Box->Position, ColorPink);
    }

    for (i32 SphereIndex = 0; SphereIndex < gSphereCount; ++SphereIndex)
    {
        sphere *Sphere = &gSpheres[SphereIndex];
        glm::vec3 Color = ColorGrey;
        if (gControlShapeType == 1 && gControlShape == SphereIndex)
        {
            if (!Sphere->IsColliding)
            {
                Color = ColorYellow;
            }
            else
            {
                Color = ColorOrange;
            }
        }

        DEBUG_RenderSphere(DebugCollisionShader, Sphere, Color);
        DEBUG_AddDebugPoint(&DebugPointsStorage, Sphere->Position, ColorPink);
    }

    for (i32 PlaneIndex = 0; PlaneIndex < gPlaneCount; ++PlaneIndex)
    {
        plane *Plane = &gPlanes[PlaneIndex];
        glm::vec3 Color = ColorGrey;

        DEBUG_RenderPlane(DebugCollisionShader, Plane, Color);
        glm::vec3 PlaneOrigin = Plane->Normal * Plane->Distance;
        DEBUG_AddDebugPoint(&DebugPointsStorage, PlaneOrigin, ColorPink);
        DEBUG_AddDebugVector(&DebugVectorsStorage, PlaneOrigin, PlaneOrigin + Plane->Normal * 0.3f, ColorRed);
    }

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

aabb
DEBUG_GenerateBox(glm::vec3 Position, glm::vec3 Extents)
{
    glm::vec3 Vertices[8];
    i32 Indices[36];
    DEBUG_GetBoxVerticesAndIndices(Extents, Vertices, Indices);

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

    aabb Result{};
    Result.Position = Position;
    Result.Extents = Extents;
    Result.IndexCount = 36;
    Result.VAO = VAO;
    return Result;
}

sphere
DEBUG_GenerateSphere(glm::vec3 Position, f32 Radius)
{
    glm::vec3 Vertices[SPHERE_RING_COUNT * SPHERE_SECTOR_COUNT];
    i32 Indices[SPHERE_RING_COUNT * SPHERE_SECTOR_COUNT * 6];
    i32 RingCount = SPHERE_RING_COUNT;
    i32 SectorCount = SPHERE_SECTOR_COUNT;
    DEBUG_GetSphereVerticesAndIndices(Radius, RingCount, SectorCount, Vertices, Indices);

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

    sphere Result{};
    Result.Position = Position;
    Result.Radius = Radius;
    Result.IndexCount = IndexCount;
    Result.VAO = VAO;
    return Result;
}

plane
DEBUG_GeneratePlane(glm::vec3 Normal, f32 Distance)
{
    glm::vec3 Vertices[4];
    i32 Indices[6];
    DEBUG_GetPlaneVerticesAndIndices(Vertices, Indices);

    f32 *RawData = DEBUG_GetRawVertexDataFromVec3(Vertices, 4);
    u32 VAO;
    glGenVertexArrays(1, &VAO);
    u32 VBO;
    glGenBuffers(1, &VBO);
    u32 EBO;
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * 3 * sizeof(f32), RawData, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(i32), Indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (void *) 0);
    glBindVertexArray(0);
    free(RawData);

    plane Result;
    Result.Normal = glm::normalize(Normal);
    Result.Distance = Distance;
    Result.IndexCount = 6;
    Result.VAO = VAO;
    return Result;
}

void
DEBUG_GetBoxVerticesAndIndices(glm::vec3 Extents, glm::vec3 *Out_Vertices, i32 *Out_Indices)
{
    Assert(Out_Vertices);
    Assert(Out_Indices);

    f32 HalfWidth = Extents.x;
    f32 HalfHeight = Extents.y;
    f32 HalfDepth = Extents.z;

    Out_Vertices[0].x = -HalfWidth; Out_Vertices[0].y = HalfHeight;  Out_Vertices[0].z = HalfDepth;
    Out_Vertices[1].x = -HalfWidth; Out_Vertices[1].y = -HalfHeight; Out_Vertices[1].z = HalfDepth;
    Out_Vertices[2].x = HalfWidth;  Out_Vertices[2].y = -HalfHeight; Out_Vertices[2].z = HalfDepth;
    Out_Vertices[3].x = HalfWidth;  Out_Vertices[3].y = HalfHeight;  Out_Vertices[3].z = HalfDepth;
    Out_Vertices[4].x = HalfWidth;  Out_Vertices[4].y = HalfHeight;  Out_Vertices[4].z = -HalfDepth;
    Out_Vertices[5].x = HalfWidth;  Out_Vertices[5].y = -HalfHeight; Out_Vertices[5].z = -HalfDepth;
    Out_Vertices[6].x = -HalfWidth; Out_Vertices[6].y = -HalfHeight; Out_Vertices[6].z = -HalfDepth;
    Out_Vertices[7].x = -HalfWidth; Out_Vertices[7].y = HalfHeight;  Out_Vertices[7].z = -HalfDepth;

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
DEBUG_GetSphereVerticesAndIndices(f32 Radius, i32 RingCount, i32 SectorCount, glm::vec3 *Out_Vertices, i32 *Out_Indices)
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

            Out_Vertices[VertexIndex].x = X * Radius;
            Out_Vertices[VertexIndex].y = Y * Radius;
            Out_Vertices[VertexIndex].z = Z * Radius;

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

void
DEBUG_GetPlaneVerticesAndIndices(glm::vec3 *Out_Vertices, i32 *Out_Indices)
{
    Assert(Out_Vertices);
    Assert(Out_Indices);

    f32 HalfWidth = 0.5f;
    f32 HalfDepth = 0.5f;

    Out_Vertices[0].x = -HalfWidth; Out_Vertices[0].y = 0.0f; Out_Vertices[0].z = -HalfDepth;
    Out_Vertices[1].x = -HalfWidth; Out_Vertices[1].y = 0.0f; Out_Vertices[1].z =  HalfDepth;
    Out_Vertices[2].x =  HalfWidth; Out_Vertices[2].y = 0.0f; Out_Vertices[2].z =  HalfDepth;
    Out_Vertices[3].x =  HalfWidth; Out_Vertices[3].y = 0.0f; Out_Vertices[3].z = -HalfDepth;

    i32 Indices[] = {
        0, 1, 3,  3, 1, 2
    };
    memcpy_s(Out_Indices, 6 * sizeof(i32), Indices, 6 * sizeof(i32));
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
DEBUG_MoveBox(aabb *Box, f32 DeltaTime, glm::vec3 Velocity, bool ForceResolve)
{
    Box->Position += Velocity * DeltaTime;

    bool IsCollidingWithAny = false;
    for (i32 BoxIndex = 0; BoxIndex < gBoxCount; ++BoxIndex)
    {
        if (BoxIndex != gControlShape)
        {
            glm::vec3 PositionAdjustment(0.0f);
            bool IsColliding = DEBUG_CheckCollision2AABB(*Box, gBoxes[BoxIndex], &PositionAdjustment);
            if (IsColliding)
            {
                IsColliding = !DEBUG_ResolveCollision(&Box->Position, PositionAdjustment, ForceResolve);
            }
            IsCollidingWithAny |= IsColliding;
        }
    }
    for (i32 SphereIndex = 0; SphereIndex < gSphereCount; ++SphereIndex)
    {
        glm::vec3 PositionAdjustment(0.0f);
        bool IsColliding = DEBUG_CheckCollisionSphereAABB(gSpheres[SphereIndex], *Box, &PositionAdjustment);
        if (IsColliding)
        {
            PositionAdjustment *= -1.0f;
            IsColliding = !DEBUG_ResolveCollision(&Box->Position, PositionAdjustment, ForceResolve);
        }
        IsCollidingWithAny |= IsColliding;
    }
    Box->IsColliding = IsCollidingWithAny;
}

void
DEBUG_MoveSphere(sphere *Sphere, f32 DeltaTime, glm::vec3 Velocity, bool ForceResolve)
{
    Sphere->Position += Velocity * DeltaTime;

    bool IsCollidingWithAny = false;
    for (i32 BoxIndex = 0; BoxIndex < gBoxCount; ++BoxIndex)
    {
        glm::vec3 PositionAdjustment(0.0f);
        bool IsColliding = DEBUG_CheckCollisionSphereAABB(*Sphere, gBoxes[BoxIndex], &PositionAdjustment);
        if (IsColliding)
        {
            IsColliding = !DEBUG_ResolveCollision(&Sphere->Position, PositionAdjustment, ForceResolve);
        }
        IsCollidingWithAny |= IsColliding;
    }
    for (i32 SphereIndex = 0; SphereIndex < gSphereCount; ++SphereIndex)
    {
        if (gControlShape != SphereIndex)
        {
            glm::vec3 PositionAdjustment(0.0f);
            bool IsColliding = DEBUG_CheckCollision2Sphere(*Sphere, gSpheres[SphereIndex], &PositionAdjustment);
            if (IsColliding)
            {
                IsColliding = !DEBUG_ResolveCollision(&Sphere->Position, PositionAdjustment, ForceResolve);
            }
            IsCollidingWithAny |= IsColliding;
        }
    }
    Sphere->IsColliding = IsCollidingWithAny;
}

bool
DEBUG_CheckCollision2AABB(aabb A, aabb B, glm::vec3 *Out_PositionAdjustment)
{
    glm::vec3 ACenter = A.Position;
    glm::vec3 AExtents = A.Extents;
    glm::vec3 AMin = ACenter - AExtents;
    glm::vec3 AMax = ACenter + AExtents;

    glm::vec3 BCenter = B.Position;
    glm::vec3 BExtents = B.Extents;
    glm::vec3 BMin = BCenter - BExtents;
    glm::vec3 BMax = BCenter + BExtents;

    bool IsColliding = true;
    for (i32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
    {
        IsColliding &= (AMin[AxisIndex] < BMax[AxisIndex] &&
                        AMax[AxisIndex] > BMin[AxisIndex]);
    }

    if (Out_PositionAdjustment && IsColliding)
    {
        f32 MinPenetration = FLT_MAX;
        i32 MinPenetrationAxis = 0;
        bool MinPenetrationFromBMaxSide = false;
        for (i32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
        {
            f32 PenetrationFromBMaxSide = BMax[AxisIndex] - AMin[AxisIndex];
            Assert(PenetrationFromBMaxSide >= 0.0f);
            if (PenetrationFromBMaxSide < MinPenetration)
            {
                MinPenetration = PenetrationFromBMaxSide;
                MinPenetrationAxis = AxisIndex;
                MinPenetrationFromBMaxSide = true;
            }

            f32 PenetrationFromBMinSide = AMax[AxisIndex] - BMin[AxisIndex];
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
        *Out_PositionAdjustment = glm::vec3(0.0f);
        (*Out_PositionAdjustment)[MinPenetrationAxis] = Sign * MinPenetration;
    }

    return IsColliding;
}

bool
DEBUG_CheckCollisionSphereAABB(sphere A, aabb B, glm::vec3 *Out_PositionAdjustment)
{
    glm::vec3 ACenter = A.Position;
    f32 ARadius = A.Radius;

    glm::vec3 BCenter = B.Position;
    glm::vec3 BExtents = B.Extents;
    glm::vec3 BMin = BCenter - BExtents;
    glm::vec3 BMax = BCenter + BExtents;

    glm::vec3 vBCenterACenter = ACenter - BCenter;
    glm::vec3 pClosestOnB = vBCenterACenter;
    bool IsACenterOutsideB = DEBUG_GetClosestPointOnBoxSurface(B, &pClosestOnB);
    glm::vec3 pClosestOnBGlobal = pClosestOnB + BCenter;
    glm::vec3 vACenterClosestOnB = pClosestOnBGlobal- ACenter;
    f32 LengthSqr = GetSquaredVectorLength(vACenterClosestOnB);
    f32 RadiusSqr = ARadius * ARadius;
    bool IsClosestOnBCloserThanRadius = (RadiusSqr - LengthSqr) > 0.000001f;
    bool IsColliding = !IsACenterOutsideB || IsClosestOnBCloserThanRadius;

    if (Out_PositionAdjustment && IsColliding)
    {
        f32 Sign = IsACenterOutsideB ? 1.0f : -1.0f;
        glm::vec3 pClosestOnA = Sign * glm::normalize(vACenterClosestOnB) * ARadius;
        glm::vec3 pClosestOnAGlobal = pClosestOnA + ACenter;

        *Out_PositionAdjustment = glm::vec3(0.0f);
        (*Out_PositionAdjustment) = pClosestOnBGlobal - pClosestOnAGlobal;
    }

    return IsColliding;
}

bool
DEBUG_CheckCollision2Sphere(sphere A, sphere B, glm::vec3 *Out_PositionAdjustment)
{
    glm::vec3 ACenter = A.Position;
    f32 ARadius = A.Radius;
    
    glm::vec3 BCenter = B.Position;
    f32 BRadius = B.Radius;

    glm::vec3 vBCenterACenter = ACenter - BCenter;
    f32 LengthSqr = GetSquaredVectorLength(vBCenterACenter);
    f32 RadiiSqr = (ARadius + BRadius) * (ARadius + BRadius);
    bool IsColliding = (RadiiSqr - LengthSqr) > 0.000001f;

    if (Out_PositionAdjustment && IsColliding)
    {
        glm::vec3 Direction = glm::normalize(vBCenterACenter);
        f32 Distance = ARadius + BRadius - glm::length(vBCenterACenter);
        *Out_PositionAdjustment = Distance * Direction;

        DEBUG_AddDebugVector(&DebugVectorsStorage, ACenter, BCenter, ColorGrey);
    }

    return IsColliding;
}

bool
DEBUG_ResolveCollision(glm::vec3 *Position, glm::vec3 Adjustment, bool ForceResolve)
{
    // NOTE: This function might only be necessary while debugging
    if (gResolveCollisions || ForceResolve)
    {
        DEBUG_AddDebugVector(&DebugVectorsStorage, *Position, *Position + 0.3f * glm::normalize(Adjustment), ColorBlue);
        *Position += Adjustment;
        return true;
    }
    else
    {
        DEBUG_AddDebugVector(&DebugVectorsStorage, *Position, *Position + Adjustment, ColorBlue);
        return false;
    }
}

bool
DEBUG_GetClosestPointOnBoxSurface(aabb Box, glm::vec3 *PointRelativeToCenter)
{
    bool IsPointOutsideBox = ClampVec3(PointRelativeToCenter, -Box.Extents, Box.Extents);

    if (IsPointOutsideBox)
    {
        return true;
    }
    else
    {
        f32 MinDistance = FLT_MAX;
        i32 MinDistanceAxis = 0;
        for (i32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
        {
            f32 Distance = fabs(Box.Extents[AxisIndex]) - fabs((*PointRelativeToCenter)[AxisIndex]);
            Assert(Distance >= 0.0f);
            if (Distance < MinDistance)
            {
                MinDistance = Distance;
                MinDistanceAxis = AxisIndex;
            }
        }

        f32 Sign = (*PointRelativeToCenter)[MinDistanceAxis] < 0.0f ? -1.0f : 1.0f;
        (*PointRelativeToCenter)[MinDistanceAxis] += Sign * MinDistance;
        return false;
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
DEBUG_RenderBox(u32 Shader, aabb *Box, glm::vec3 Color)
{
    glm::mat4 Model(1.0f);
    Model = glm::translate(Model, Box->Position);
    SetUniformMat4F(Shader, "Model", false, glm::value_ptr(Model));

    SetUniformVec3F(Shader, "Color", false, &Color[0]);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(3);

    glBindVertexArray(Box->VAO);
    glDrawElements(GL_TRIANGLES, Box->IndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void
DEBUG_RenderSphere(u32 Shader, sphere *Sphere, glm::vec3 Color)
{
    glm::mat4 Model(1.0f);
    Model = glm::translate(Model, Sphere->Position);
    SetUniformMat4F(Shader, "Model", false, glm::value_ptr(Model));

    SetUniformVec3F(Shader, "Color", false, &Color[0]);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(3);

    glBindVertexArray(Sphere->VAO);
    glDrawElements(GL_TRIANGLES, Sphere->IndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void
DEBUG_RenderPlane(u32 Shader, plane *Plane, glm::vec3 Color)
{
    glm::mat4 Model(1.0f);
    Model = glm::translate(Model, Plane->Distance * Plane->Normal);
    glm::vec3 U(1.0f, 0.0f, 0.0f);
    glm::vec3 Projection = glm::dot(Plane->Normal, U) * U;
    glm::vec3 V = glm::normalize(Plane->Normal - Projection);
    glm::mat3 Rotation(Plane->Normal, U, V);
    Model *= glm::mat4(Rotation);
    SetUniformMat4F(Shader, "Model", false, glm::value_ptr(Model));

    SetUniformVec3F(Shader, "Color", false, &Color[0]);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(3);

    glBindVertexArray(Plane->VAO);
    glDrawElements(GL_TRIANGLES, Plane->IndexCount, GL_UNSIGNED_INT, 0);
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
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    DebugVectors->VectorCount = 0;
}

#endif