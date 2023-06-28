#ifndef MODEL_H
#define MODEL_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <vector>

#include "Common.h"

struct Mesh
{
    u32 vao;
    u32 indexCount;
    u32 diffuseMapID;
    u32 specularMapID;
    u32 emissionMapID;
    u32 normalMapID;
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

#define MAX_BONE_DEPTH 16
struct Bone
{
    std::string name;
    i32 pathToRoot[MAX_BONE_DEPTH];
    i32 pathNodes;
    glm::mat4 transformToParent;
    glm::mat4 inverseBindTransform;

    std::vector<PositionKey> positionKeys;
    std::vector<ScalingKey> scalingKeys;
    std::vector<RotationKey> rotationKeys;
};

struct AnimationData
{
    f32 ticksDuration;
    f32 ticksPerSecond;

    f32 currentTicks;
    bool isRunning = true;
    bool isPaused = false;
    bool isLooped = true;
};

struct SkinnedModel
{
    std::vector<Mesh> meshes;
    std::vector<Bone> bones;

    AnimationData animation;
};

std::vector<Mesh> loadModel(const char *path);
SkinnedModel debugModelGLTF(const char *path);
void render(SkinnedModel *model, u32 shader, f32 deltaTime);

#endif
