#ifndef MODEL_H
#define MODEL_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <vector>

#include "Common.h"

struct mesh
{
    u32 VAO;
    u32 IndexCount;
    u32 DiffuseMapID;
    u32 SpecularMapID;
    u32 EmissionMapID;
    u32 NormalMapID;
};

#define MAX_BONE_NAME_LENGTH 32
#define MAX_BONE_CHILDREN 8
struct bone
{
    i32 ID;
    i32 ParentID;
    i32 ChildrenIDs[MAX_BONE_CHILDREN];
    i32 ChildrenCount;
    glm::mat4 TransformToParent;
    glm::mat4 InverseBindTransform;
    char Name[MAX_BONE_NAME_LENGTH];
};

struct animation_key
{
    glm::vec3 Position;
    glm::quat Rotation;
    glm::vec3 Scale;
};

struct animation_data
{
    f32 TicksDuration;
    f32 TicksPerSecond;

    f32 CurrentTicks;
    bool IsRunning = true;
    bool IsPaused = false;
    bool IsLooped = true;

    i32 KeyCount;
    i32 ChannelCount;
    f32 *KeyTimes;
    animation_key *Keys;

    glm::mat4 *TransientChannelTransformData;
};

struct skinned_model
{
    i32 MeshCount;
    mesh *Meshes;

    i32 BoneCount;
    bone *Bones;

    animation_data AnimationData;
};

#define POSITIONS_PER_VERTEX 3
#define UVS_PER_VERTEX 2
#define NORMALS_PER_VERTEX 3
#define TANGENTS_PER_VERTEX 3
#define BITANGENTS_PER_VERTEX 3
#define MAX_BONES_PER_VERTEX 4
struct mesh_internal_data
{
    u8 *Data;

    i32 VertexCount;
    i32 IndexCount;
    
    f32 *Positions;
    f32 *UVs;
    f32 *Normals;
    f32 *Tangents;
    f32 *Bitangents;
    i32 *BoneIDs;
    f32 *BoneWeights;

    i32 *Indices;
};


//std::vector<mesh> loadModel(const char *Path);
skinned_model LoadSkinnedModel(const char *Path);
void Render(skinned_model *Model, u32 Shader, f32 DeltaTime);

#endif