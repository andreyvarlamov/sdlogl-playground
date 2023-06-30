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

struct PositionKey
{
    f32 time;
    glm::vec3 position;
};

struct ScalingKey
{
    f32 time;
    glm::vec3 scale;
};

struct RotationKey
{
    f32 time;
    glm::quat rotation;
};

#define MAX_BONE_NAME_LENGTH 32
struct bone
{
    i32 ID;
    i32 ParentID;
    glm::mat4 TransformToParent;
    glm::mat4 InverseBindTransform;
    char Name[MAX_BONE_NAME_LENGTH];

    //std::vector<PositionKey> positionKeys;
    //std::vector<ScalingKey> scalingKeys;
    //std::vector<RotationKey> rotationKeys;
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
};

struct skinned_model
{
    i32 MeshCount;
    mesh *Meshes;

    i32 BoneCount;
    bone *Bones;

    animation_data AnimationData;
};

struct vertex_data
{
    glm::vec3 Position;
    glm::vec2 UVs;
    glm::vec3 Normal;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    glm::ivec4 BoneIDs;
    glm::vec4 BoneWeights;
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
