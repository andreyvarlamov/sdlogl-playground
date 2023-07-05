#include "Model.h"

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>
#include <iostream>
#include <queue>
#include <stack>
#include <unordered_map>

#include "Util.h"

// ------------------------------
// INTERNAL FUNCTION DECLARATIONS
// ------------------------------

// Assimp helpers
// --------------

static const aiScene *
ASSIMP_ImportFile(const char *Path);
static void
ASSIMP_GetArmatureInfo(aiNode *RootNode, aiNode **Out_ArmatureNode, i32 *Out_BoneCount);
static void
ASSIMP_GetArmatureInfoHelper(aiNode *Node, aiNode **Out_ArmatureNode, i32 *Out_BoneCount);
static bone *
ASSIMP_ParseBones(aiNode *ArmatureNode, i32 BoneCount);
static void
ASSIMP_ParseMeshVertexIndexData(aiMesh *AssimpMesh, mesh_internal_data *Out_InternalData);
static void
ASSIMP_ParseMeshBoneData(aiMesh *AssimpMesh, skinned_model *Out_Model, mesh_internal_data *Out_InternalData);
static animation
ASSIMP_ParseAnimation(aiAnimation *AssimpAnimation, bone *Bones, i32 BoneCount);
static inline glm::mat4
ASSIMP_Mat4ToGLM(aiMatrix4x4 AssimpMat);
static inline glm::vec3
ASSIMP_Vec3ToGLM(aiVector3D AssimpVector);
static inline glm::quat
ASSIMP_QuatToGLM(aiQuaternion AssimpQuat);
static void
ASSIMP_GetTextureFilename(aiMaterial *AssimpMaterial, aiTextureType TextureType,
                               char *Out_Filename, i32 *Out_FilenameCount,
                               i32 FilenameBufferSize);

// Mesh data prep
// --------------

static mesh_internal_data
InitializeMeshInternalData(i32 VertexCount, i32 IndexCount, bool IncludeBones);
static void
FreeMeshInternalData(mesh_internal_data *MeshInternalData);
static void
PrepareMeshRenderData(mesh_internal_data MeshInternalData, mesh *Out_Mesh);
static void
PrepareSkinnedMeshRenderData(mesh_internal_data MeshInternalData, mesh *Out_Mesh);
static void
LoadTexturesForMesh(mesh *Mesh, const char *ModelPath, aiMaterial *AssimpMaterial);

// Render helpers
// --------------

static inline void
RenderMeshList(mesh *Meshes, i32 MeshCount);

// Render helpers (animation)
// --------------------------

static inline void
UpdateAnimationState(animation *Animation, f32 DeltaTime);
static inline i32
FindNextAnimationKey(animation *Animation);
static inline f32
CalculateLerpRatioBetweenTwoFrames(animation *Animation, i32 NextKey);
static inline animation_key
LerpAnimationKeys(animation_key KeyA, animation_key KeyB, f32 LerpRatio);
static inline animation_key
GetRestAnimationKeyForBone(bone Bone);
static inline glm::mat4
GetTransformationForAnimationKey(animation_key Key);

// -----------------------------
// EXTERNAL FUNCTION DEFINITIONS
// -----------------------------

// Model loading
// -------------

model
LoadModel(const char *Path)
{
    std::cout << "Loading model at: " << Path << '\n';

    model Model{ };

    const aiScene *AssimpScene = ASSIMP_ImportFile(Path);

    // Scene mesh data
    // ---------------
    Model.MeshCount = AssimpScene->mNumMeshes;
    // TODO: LEAK
    Model.Meshes = (mesh *) calloc(1, Model.MeshCount * sizeof(mesh));
    Assert(Model.Meshes);

    for (i32 MeshIndex = 0; MeshIndex < Model.MeshCount; ++MeshIndex)
    {
        aiMesh *AssimpMesh = AssimpScene->mMeshes[MeshIndex];

        mesh Mesh{ };

        i32 VertexCount = AssimpMesh->mNumVertices;
        i32 IndexCount = AssimpMesh->mNumFaces * 3;
        mesh_internal_data InternalData = InitializeMeshInternalData(VertexCount, IndexCount, false);

        ASSIMP_ParseMeshVertexIndexData(AssimpMesh, &InternalData);

        PrepareMeshRenderData(InternalData, &Mesh);

        FreeMeshInternalData(&InternalData);

        LoadTexturesForMesh(&Mesh, Path, AssimpScene->mMaterials[AssimpMesh->mMaterialIndex]);

        Model.Meshes[MeshIndex] = Mesh;
    }

    // Done with assimp data, free
    // ---------------------------
    aiReleaseImport(AssimpScene);

    return Model;
}

skinned_model
LoadSkinnedModel(const char *Path)
{
    std::cout << "Loading skinned model at: " << Path << '\n';

    skinned_model Model{ };

    const aiScene *AssimpScene = ASSIMP_ImportFile(Path);

    // Scene armature data
    // -------------------
    aiNode *ArmatureNode = 0;
    i32 BoneCount = 0;
    ASSIMP_GetArmatureInfo(AssimpScene->mRootNode, &ArmatureNode, &BoneCount);
    Assert(ArmatureNode);
    Assert(BoneCount > 0);
    Model.BoneCount = BoneCount;
    Model.Bones = ASSIMP_ParseBones(ArmatureNode, BoneCount);

    // Scene mesh data
    // ---------------
    Model.MeshCount = AssimpScene->mNumMeshes;
    // TODO: LEAK
    Model.Meshes = (mesh *) calloc(1, Model.MeshCount * sizeof(mesh));
    Assert(Model.Meshes);

    for (i32 MeshIndex = 0; MeshIndex < Model.MeshCount; ++MeshIndex)
    {
        aiMesh *AssimpMesh = AssimpScene->mMeshes[MeshIndex];
        
        mesh Mesh{ };

        i32 VertexCount = AssimpMesh->mNumVertices;
        i32 IndexCount = AssimpMesh->mNumFaces * 3;
        mesh_internal_data InternalData = InitializeMeshInternalData(VertexCount, IndexCount, true);

        ASSIMP_ParseMeshVertexIndexData(AssimpMesh, &InternalData);

        ASSIMP_ParseMeshBoneData(AssimpMesh, &Model, &InternalData);

        PrepareSkinnedMeshRenderData(InternalData, &Mesh);

        FreeMeshInternalData(&InternalData);

        LoadTexturesForMesh(&Mesh, Path, AssimpScene->mMaterials[AssimpMesh->mMaterialIndex]);
        
        Model.Meshes[MeshIndex] = Mesh;
    }

    // Scene animation data
    // --------------------
    Model.AnimationCount = AssimpScene->mNumAnimations;
    //TODO: LEAK
    Model.Animations = (animation *) calloc(1, Model.AnimationCount * sizeof(animation));
    Assert(Model.Animations);

    i32 ModelAnimationChannelCount = AssimpScene->mAnimations[0]->mNumChannels;
    for (i32 AnimationIndex = 0; AnimationIndex < Model.AnimationCount; ++AnimationIndex)
    {
        aiAnimation *AssimpAnimation = AssimpScene->mAnimations[AnimationIndex];
        
        // NOTE: An assumption I'm making: all animations for the same model have the same number of channels
        Assert(AssimpAnimation->mNumChannels == ModelAnimationChannelCount);
        
        Model.Animations[AnimationIndex] = ASSIMP_ParseAnimation(AssimpAnimation, Model.Bones, Model.BoneCount);
    }

    // TODO: LEAK
    Model.AnimationState.TransientChannelTransformData = 
        (glm::mat4 *) calloc(1, ModelAnimationChannelCount * sizeof(glm::mat4));
    Assert(Model.AnimationState.TransientChannelTransformData);

    // Done with assimp data, free
    // ---------------------------
    aiReleaseImport(AssimpScene);

    return Model;
}

// Model rendering
// ---------------

void
RenderModel(model *Model, u32 Shader)
{
    glUseProgram(Shader);

    // Render model's meshes
    // ---------------------
    RenderMeshList(Model->Meshes, Model->MeshCount);

    glUseProgram(0);
}

void
RenderSkinnedModel(skinned_model *Model, u32 Shader, f32 DeltaTime)
{
    glUseProgram(Shader);

    // Process animation transforms
    // ----------------------------
    animation *CurrentAnimationA = &Model->Animations[Model->AnimationState.CurrentAnimationA];
    animation *CurrentAnimationB = &Model->Animations[Model->AnimationState.CurrentAnimationB];

    UpdateAnimationState(CurrentAnimationA, DeltaTime);
    UpdateAnimationState(CurrentAnimationB, DeltaTime);

    // Find current animation keyframes
    i32 NextKeyA = FindNextAnimationKey(CurrentAnimationA);
    i32 NextKeyB = FindNextAnimationKey(CurrentAnimationB);

    // Find the lerp ratio that will be used to determine the place between the current key and the next key
    f32 LerpRatioA = CalculateLerpRatioBetweenTwoFrames(CurrentAnimationA, NextKeyA);
    f32 LerpRatioB = CalculateLerpRatioBetweenTwoFrames(CurrentAnimationB, NextKeyB);

    // Calculate animation transforms for each of the animation channels
    // NOTE: All animations have the same channel count
    i32 ChannelCount = CurrentAnimationA->ChannelCount;
    for (i32 ChannelIndex = 0; ChannelIndex < ChannelCount; ++ChannelIndex)
    {
        i32 BoneID = ChannelIndex + 1;

        animation_key InterpolatedKeyA = LerpAnimationKeys(CurrentAnimationA->Keys[(NextKeyA-1)*ChannelCount + ChannelIndex],
                                                           CurrentAnimationA->Keys[NextKeyA*ChannelCount + ChannelIndex],
                                                           LerpRatioA);
        // NOTE: This is causing some weird jumping in some animations
        //       E.g. the second shape in atlbeta10.gltf (BONETREE.blend)
        //       Something with 360 rotation?
        // TODO: Investigate (should be easier when there's texture loaded and debugging ui)
        animation_key InterpolatedKeyB = LerpAnimationKeys(CurrentAnimationB->Keys[(NextKeyB-1)*ChannelCount + ChannelIndex],
                                                           CurrentAnimationB->Keys[NextKeyB*ChannelCount + ChannelIndex],
                                                           LerpRatioB);
        //animation_key InterpolatedKeyA = GetRestAnimationKeyForBone(Model->Bones[BoneID]);

        // Blend between 2 animations
        animation_key BlendedKey = LerpAnimationKeys(InterpolatedKeyA, InterpolatedKeyB, Model->AnimationState.BlendingFactor);

        glm::mat4 Transform = GetTransformationForAnimationKey(BlendedKey);

        if (Model->Bones[BoneID].ParentID > 0)
        {
            i32 ParentBoneChannelID = Model->Bones[BoneID].ParentID - 1;
            Assert(ParentBoneChannelID < ChannelCount);
            Transform = Model->AnimationState.TransientChannelTransformData[ParentBoneChannelID] * Transform;
        }

        Model->AnimationState.TransientChannelTransformData[ChannelIndex] = Transform;
    }

    // Pass the bone transforms to the shader
    // Skip bone #0 (DummyBone)
    for (i32 BoneIndex = 1; BoneIndex < Model->BoneCount; ++BoneIndex)
    {
        i32 ChannelID = BoneIndex - 1;
        Assert(ChannelID < ChannelCount);
        glm::mat4 Transform = (Model->AnimationState.TransientChannelTransformData[ChannelID] *
                               Model->Bones[BoneIndex].InverseBindTransform);

        // TODO: This should probably use a UBO...
        char LocationStringBuffer[32] = { };
        // TODO: This is probably very slow...
        sprintf_s(LocationStringBuffer, "BoneTransforms[%d]", BoneIndex);
        //Transform = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(Shader, LocationStringBuffer), 1, GL_FALSE, glm::value_ptr(Transform));
    }

    // Render model's meshes
    // ---------------------
    RenderMeshList(Model->Meshes, Model->MeshCount);

    glUseProgram(0);
}

// -----------------------------
// INTERNAL FUNCTION DEFINITIONS
// -----------------------------

static const aiScene *
ASSIMP_ImportFile(const char *Path)
{
    const aiScene *AssimpScene = aiImportFile(Path,
                                              aiProcess_CalcTangentSpace |
                                              aiProcess_Triangulate |
                                              aiProcess_JoinIdenticalVertices |
                                              aiProcess_FlipUVs);

    Assert(AssimpScene);
    Assert(!(AssimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE));
    Assert(AssimpScene->mRootNode);

    if (!AssimpScene || AssimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !AssimpScene->mRootNode)
    {
        // TODO: Logging
        return 0;
    }
}

static void
ASSIMP_GetArmatureInfo(aiNode *RootNode, aiNode **Out_ArmatureNode, i32 *Out_BoneCount)
{
    // Allocate for one extra bone, so the bone indexed 0 can count as no bone
    // TODO: Deal with assimp's "neutral_bone" for some models
    *Out_BoneCount = 1;
    *Out_ArmatureNode = 0;

    // TODO: Now I want to get rid of recursion again...
    ASSIMP_GetArmatureInfoHelper(RootNode, Out_ArmatureNode, Out_BoneCount);
}

static void
ASSIMP_GetArmatureInfoHelper(aiNode *Node, aiNode **Out_ArmatureNode, i32 *Out_BoneCount)
{
    if (strcmp(Node->mName.C_Str(), "Armature") == 0)
    {
        *Out_ArmatureNode = Node;
    }
    else if (*Out_ArmatureNode)
    {
        ++(*Out_BoneCount);
    }

    for (i32 Index = 0; Index < (i32) Node->mNumChildren; ++Index)
    {
        ASSIMP_GetArmatureInfoHelper(Node->mChildren[Index], Out_ArmatureNode, Out_BoneCount);
    }
}

static bone *
ASSIMP_ParseBones(aiNode *ArmatureNode, i32 BoneCount)
{
    Assert(BoneCount > 0);
    Assert(BoneCount < 128);

    i32 CurrentBoneIndex = 0;
    // TODO: LEAK
    bone *Bones = (bone *) calloc(1, (BoneCount) * sizeof(bone));
    Assert(Bones);

    aiNode *NodeQueue[128] = { };
    i32 ParentIDHelperQueue[128] = { };
    i32 QueueStart = 0;
    i32 QueueEnd = 0;
    NodeQueue[QueueEnd++] = ArmatureNode;

    while (QueueStart != QueueEnd)
    {
        aiNode *CurrentNode = NodeQueue[QueueStart];

        bone Bone { };
        // First bone is a dummy bone
        if (CurrentBoneIndex == 0)
        {
            strncpy_s(Bone.Name, "DummyBone", MAX_INTERNAL_NAME_LENGTH -1);
        }
        else
        {
            strncpy_s(Bone.Name, CurrentNode->mName.C_Str(), MAX_INTERNAL_NAME_LENGTH -1);
            Bone.ParentID = ParentIDHelperQueue[QueueStart];
        }
        Bone.ID = CurrentBoneIndex;
        Bone.TransformToParent = ASSIMP_Mat4ToGLM(CurrentNode->mTransformation);

        for (i32 ChildIndex = 0; ChildIndex < (i32) CurrentNode->mNumChildren; ++ChildIndex)
        {
            NodeQueue[QueueEnd] = CurrentNode->mChildren[ChildIndex];
            ParentIDHelperQueue[QueueEnd] = CurrentBoneIndex;
            Bone.ChildrenIDs[Bone.ChildrenCount++] = QueueEnd;
            ++QueueEnd;
        }

        Bones[CurrentBoneIndex++] = Bone;

        ++QueueStart;
    }

    return Bones;
}

static void
ASSIMP_ParseMeshVertexIndexData(aiMesh *AssimpMesh, mesh_internal_data *Out_InternalData)
{
    i32 VertexCount = AssimpMesh->mNumVertices;

    for (i32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
    {
        for (i32 Index = 0; Index < POSITIONS_PER_VERTEX; ++Index)
        {
            Out_InternalData->Positions[VertexIndex * POSITIONS_PER_VERTEX + Index] =
                AssimpMesh->mVertices[VertexIndex][Index];
        }

        for (i32 Index = 0; Index < UVS_PER_VERTEX; ++Index)
        {
            Out_InternalData->UVs[VertexIndex * UVS_PER_VERTEX + Index] =
                AssimpMesh->mTextureCoords[0][VertexIndex][Index];
        }

        for (i32 Index = 0; Index < NORMALS_PER_VERTEX; ++Index)
        {
            Out_InternalData->Normals[VertexIndex * NORMALS_PER_VERTEX + Index] =
                AssimpMesh->mNormals[VertexIndex][Index];
        }

        for (i32 Index = 0; Index < TANGENTS_PER_VERTEX; ++Index)
        {
            Out_InternalData->Tangents[VertexIndex * TANGENTS_PER_VERTEX + Index] =
                AssimpMesh->mTangents[VertexIndex][Index];
        }

        for (i32 Index = 0; Index < BITANGENTS_PER_VERTEX; ++Index)
        {
            Out_InternalData->Bitangents[VertexIndex * BITANGENTS_PER_VERTEX + Index] =
                AssimpMesh->mBitangents[VertexIndex][Index];
        }
    }

    i32 *IndexCursor = Out_InternalData->Indices;
    for (i32 FaceIndex = 0; FaceIndex < (i32) AssimpMesh->mNumFaces; ++FaceIndex)
    {
        aiFace *AssimpFace = &AssimpMesh->mFaces[FaceIndex];
        for (i32 Index = 0; Index < (i32) AssimpFace->mNumIndices; ++Index)
        {
            Assert(IndexCursor < Out_InternalData->Indices + Out_InternalData->IndexCount);
            *IndexCursor++ = AssimpFace->mIndices[Index];
        }
    }
}

static void
ASSIMP_ParseMeshBoneData(aiMesh *AssimpMesh, skinned_model *Out_Model, mesh_internal_data *Out_InternalData)
{
    for (i32 AssimpBoneIndex = 0; AssimpBoneIndex < (i32) AssimpMesh->mNumBones; ++AssimpBoneIndex)
    {
        aiBone *AssimpBone = AssimpMesh->mBones[AssimpBoneIndex];

        i32 InternalBoneID = 0;

        for (i32 SearchBoneIndex = 0; SearchBoneIndex < Out_Model->BoneCount; ++SearchBoneIndex)
        {
            if (strcmp(Out_Model->Bones[SearchBoneIndex].Name, AssimpBone->mName.C_Str()) == 0)
            {
                InternalBoneID = SearchBoneIndex;
            }
        }

        Out_Model->Bones[InternalBoneID].InverseBindTransform = ASSIMP_Mat4ToGLM(AssimpBone->mOffsetMatrix);

        for (i32 WeightIndex = 0; WeightIndex < (i32) AssimpBone->mNumWeights; ++WeightIndex)
        {
            aiVertexWeight *AssimpWeight = &AssimpBone->mWeights[WeightIndex];

            if (AssimpWeight->mWeight > 0.0f)
            {
                bool SpaceForBoneFound = false;
                for (i32 BonePerVertexOffset = 0;
                     BonePerVertexOffset < MAX_BONES_PER_VERTEX;
                     ++BonePerVertexOffset)
                {
                    i32 VertexBonePosition = AssimpWeight->mVertexId * MAX_BONES_PER_VERTEX + BonePerVertexOffset;
                    if (Out_InternalData->BoneIDs[VertexBonePosition] == 0)
                    {
                        Out_InternalData->BoneIDs[VertexBonePosition] = InternalBoneID;
                        Out_InternalData->BoneWeights[VertexBonePosition] = AssimpWeight->mWeight;
                        SpaceForBoneFound = true;
                        break;
                    }
                }
                Assert(SpaceForBoneFound);
            }
        }
    }
}

static animation
ASSIMP_ParseAnimation(aiAnimation *AssimpAnimation, bone *Bones, i32 BoneCount)
{
    animation Result{ };

    i32 KeyCount = 0;
    i32 ChannelCount = AssimpAnimation->mNumChannels;
    f32 *AnimationKeyTimes = 0;
    animation_key *AnimationKeys = 0;
    bool firstChannel = true;
    for (i32 ChannelIndex = 0; ChannelIndex < ChannelCount; ++ChannelIndex)
    {
        aiNodeAnim *AssimpAnimationChannel = AssimpAnimation->mChannels[ChannelIndex];

        // Find the bone ID corresponding to this node
        // So that the animation nodes are in the same
        // order as bones.
        // -------------------------------------------
        i32 BoneID = 0;
        for (i32 BoneIndex = 0; BoneIndex < BoneCount; ++BoneIndex)
        {
            if (strcmp(Bones[BoneIndex].Name, AssimpAnimationChannel->mNodeName.C_Str()) == 0)
            {
                BoneID = BoneIndex;
            }
        }
        Assert(BoneID > 0);

        // Initialize key data structures on the first loop
        // ------------------------------------------------
        if (firstChannel)
        {
            KeyCount = AssimpAnimationChannel->mNumPositionKeys;
            // TODO: LEAK
            AnimationKeyTimes = (f32 *) calloc(1, KeyCount * sizeof(f32));
            Assert(AnimationKeyTimes);
            // TODO: LEAK
            AnimationKeys = (animation_key *) calloc(1, KeyCount * ChannelCount * sizeof(animation_key));
            Assert(AnimationKeys);
            for (i32 KeyIndex = 0; KeyIndex < KeyCount; ++KeyIndex)
            {
                AnimationKeyTimes[KeyIndex] = (f32) AssimpAnimationChannel->mPositionKeys[KeyIndex].mTime;
            }
            firstChannel = false;
        }

        // NOTE: An assumption I'm making: positions, rotations and scaling
        //       have the same number of keys for all channels
        Assert(KeyCount == AssimpAnimationChannel->mNumPositionKeys);
        Assert(KeyCount == AssimpAnimationChannel->mNumRotationKeys);
        Assert(KeyCount == AssimpAnimationChannel->mNumScalingKeys);

        // Process transformation data for each key of the channel
        // -------------------------------------------------------
        for (i32 KeyIndex = 0; KeyIndex < KeyCount; ++KeyIndex)
        {
            aiVectorKey *AssimpPosKey = &AssimpAnimationChannel->mPositionKeys[KeyIndex];
            aiQuatKey *AssimpRotKey = &AssimpAnimationChannel->mRotationKeys[KeyIndex];
            aiVectorKey *AssimpScaKey = &AssimpAnimationChannel->mScalingKeys[KeyIndex];

            // NOTE: an assumption I'm making: all channels have the same exact timing info
            Assert(AnimationKeyTimes[KeyIndex] == (f32) AssimpPosKey->mTime);
            Assert(AnimationKeyTimes[KeyIndex] == (f32) AssimpRotKey->mTime);
            Assert(AnimationKeyTimes[KeyIndex] == (f32) AssimpScaKey->mTime);

            animation_key Key{ };
            Key.Position = ASSIMP_Vec3ToGLM(AssimpPosKey->mValue);
            Key.Rotation = ASSIMP_QuatToGLM(AssimpRotKey->mValue);
            Key.Scale = ASSIMP_Vec3ToGLM(AssimpScaKey->mValue);

            // All animation channels should have the same order as bones for easy access
            // But BoneID = 0 is the DummyBone, no need to save animation for that
            i32 ChannelPosition = BoneID - 1;

            // TODO: Profile vs de-intereleaved
            // Interleaved A0A1B0B1C0C1...; A - Key; 0 - Channel
            AnimationKeys[KeyIndex * ChannelCount + ChannelPosition] = Key;
        }
    }

    strncpy_s(Result.Name, AssimpAnimation->mName.C_Str(), MAX_INTERNAL_NAME_LENGTH - 1);
    Result.TicksDuration = (f32) AssimpAnimation->mDuration;
    Result.TicksPerSecond = (f32) AssimpAnimation->mTicksPerSecond;
    Result.KeyCount = KeyCount;
    Result.ChannelCount = ChannelCount;
    Result.KeyTimes = AnimationKeyTimes;
    Result.Keys = AnimationKeys;

    return Result;
}

static inline glm::mat4
ASSIMP_Mat4ToGLM(aiMatrix4x4 AssimpMat)
{
    glm::mat4 Result{ };

    Result[0][0] = AssimpMat.a1; Result[0][1] = AssimpMat.b1; Result[0][2] = AssimpMat.c1; Result[0][3] = AssimpMat.d1;
    Result[1][0] = AssimpMat.a2; Result[1][1] = AssimpMat.b2; Result[1][2] = AssimpMat.c2; Result[1][3] = AssimpMat.d2;
    Result[2][0] = AssimpMat.a3; Result[2][1] = AssimpMat.b3; Result[2][2] = AssimpMat.c3; Result[2][3] = AssimpMat.d3;
    Result[3][0] = AssimpMat.a4; Result[3][1] = AssimpMat.b4; Result[3][2] = AssimpMat.c4; Result[3][3] = AssimpMat.d4;

    return Result;
}

static inline glm::vec3
ASSIMP_Vec3ToGLM(aiVector3D AssimpVector)
{
    glm::vec3 Result{ };

    Result.x = AssimpVector.x;
    Result.y = AssimpVector.y;
    Result.z = AssimpVector.z;

    return Result;
}

static inline glm::quat
ASSIMP_QuatToGLM(aiQuaternion AssimpQuat)
{
    glm::quat Result{ };

    Result.w = AssimpQuat.w;
    Result.x = AssimpQuat.x;
    Result.y = AssimpQuat.y;
    Result.z = AssimpQuat.z;

    return Result;
}

static void
ASSIMP_GetTextureFilename(aiMaterial *AssimpMaterial, aiTextureType TextureType,
                               char *Out_Filename, i32 *Out_FilenameCount,
                               i32 FilenameBufferSize)
{
    if (aiGetMaterialTextureCount(AssimpMaterial, TextureType) > 0)
    {
        aiString AssimpTextureFilename;
        aiGetMaterialString(AssimpMaterial,
                            AI_MATKEY_TEXTURE(TextureType, 0),
                            &AssimpTextureFilename);
        strncpy_s(Out_Filename, FilenameBufferSize,
                  AssimpTextureFilename.C_Str(), FilenameBufferSize - 1);

        *Out_FilenameCount = AssimpTextureFilename.length;
    }
    else
    {
        Out_Filename[0] = '\0';
        *Out_FilenameCount = 0;
    }
}

static mesh_internal_data
InitializeMeshInternalData(i32 VertexCount, i32 IndexCount, bool IncludeBones)
{
    // TODO: I think this needs to be reworked
    mesh_internal_data Result{ };

    size_t SpaceForBones = 0;
    if (IncludeBones)
    {
        SpaceForBones = MAX_BONES_PER_VERTEX * (sizeof(i32) + sizeof(f32));
    }

    size_t BytesToAllocate = VertexCount * (POSITIONS_PER_VERTEX * sizeof(f32) +
                                            UVS_PER_VERTEX * sizeof(f32) +
                                            NORMALS_PER_VERTEX * sizeof(f32) +
                                            TANGENTS_PER_VERTEX * sizeof(f32) +
                                            BITANGENTS_PER_VERTEX * sizeof(f32) +
                                            SpaceForBones +
                                            IndexCount * sizeof(i32));

    u8 *Data = (u8 *) calloc(1, BytesToAllocate);

    if (Data)
    {
        Result.Data = Data;
        Result.VertexCount = VertexCount;
        Result.IndexCount = IndexCount;
        Result.Positions = (f32 *) (Data);
        Result.UVs = (f32 *) (Result.Positions + VertexCount * POSITIONS_PER_VERTEX);
        Result.Normals = (f32 *) (Result.UVs + VertexCount * UVS_PER_VERTEX);
        Result.Tangents = (f32 *) (Result.Normals + VertexCount * NORMALS_PER_VERTEX);
        Result.Bitangents = (f32 *) (Result.Tangents + VertexCount * TANGENTS_PER_VERTEX);
        if (IncludeBones)
        {
            Result.BoneIDs = (i32 *) (Result.Bitangents + VertexCount * BITANGENTS_PER_VERTEX);
            Result.BoneWeights = (f32 *) (Result.BoneIDs + VertexCount * MAX_BONES_PER_VERTEX);
            Result.Indices = (i32 *) (Result.BoneWeights + VertexCount * MAX_BONES_PER_VERTEX);
        }
        else
        {
            Result.BoneIDs = 0;
            Result.BoneWeights = 0;
            Result.Indices = (i32 *) (Result.Bitangents + VertexCount * BITANGENTS_PER_VERTEX);
        }
    }
    else
    {
        // TODO: Logging
    }

    return Result;
}

static void
FreeMeshInternalData(mesh_internal_data *MeshInternalData)
{
    free(MeshInternalData->Data);
    memset(MeshInternalData, 0, sizeof(mesh_internal_data));
}

static void
PrepareMeshRenderData(mesh_internal_data MeshInternalData, mesh *Out_Mesh)
{
    u32 VAO;
    glGenVertexArrays(1, &VAO);
    u32 VBO;
    glGenBuffers(1, &VBO);
    u32 EBO;
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    size_t BufferSize = MeshInternalData.VertexCount * (POSITIONS_PER_VERTEX * sizeof(f32) +
                                                        UVS_PER_VERTEX * sizeof(f32) +
                                                        NORMALS_PER_VERTEX * sizeof(f32) +
                                                        TANGENTS_PER_VERTEX * sizeof(f32) +
                                                        BITANGENTS_PER_VERTEX * sizeof(f32));
    glBufferData(GL_ARRAY_BUFFER, BufferSize, MeshInternalData.Data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MeshInternalData.IndexCount * sizeof(i32), MeshInternalData.Indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, POSITIONS_PER_VERTEX, GL_FLOAT, GL_FALSE,
                          POSITIONS_PER_VERTEX * sizeof(f32),
                          (void *) ((u8 *) MeshInternalData.Positions - MeshInternalData.Data));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, UVS_PER_VERTEX, GL_FLOAT, GL_FALSE,
                          UVS_PER_VERTEX * sizeof(f32),
                          (void *) ((u8 *) MeshInternalData.UVs - MeshInternalData.Data));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, NORMALS_PER_VERTEX, GL_FLOAT, GL_FALSE,
                          NORMALS_PER_VERTEX * sizeof(f32),
                          (void *) ((u8 *) MeshInternalData.Normals - MeshInternalData.Data));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, TANGENTS_PER_VERTEX, GL_FLOAT, GL_FALSE,
                          TANGENTS_PER_VERTEX * sizeof(f32),
                          (void *) ((u8 *) MeshInternalData.Tangents - MeshInternalData.Data));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, BITANGENTS_PER_VERTEX, GL_FLOAT, GL_FALSE,
                          BITANGENTS_PER_VERTEX * sizeof(f32),
                          (void *) ((u8 *) MeshInternalData.Bitangents - MeshInternalData.Data));

    glBindVertexArray(0);

    Out_Mesh->VAO = VAO;
    Out_Mesh->IndexCount = MeshInternalData.IndexCount;
}

static void
PrepareSkinnedMeshRenderData(mesh_internal_data MeshInternalData, mesh *Out_Mesh)
{
    u32 VAO;
    glGenVertexArrays(1, &VAO);
    u32 VBO;
    glGenBuffers(1, &VBO);
    u32 EBO;
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    size_t BufferSize = MeshInternalData.VertexCount * (POSITIONS_PER_VERTEX * sizeof(f32) +
                                                        UVS_PER_VERTEX * sizeof(f32) +
                                                        NORMALS_PER_VERTEX * sizeof(f32) +
                                                        TANGENTS_PER_VERTEX * sizeof(f32) +
                                                        BITANGENTS_PER_VERTEX * sizeof(f32) +
                                                        MAX_BONES_PER_VERTEX * sizeof(i32) +
                                                        MAX_BONES_PER_VERTEX * sizeof(f32));
    glBufferData(GL_ARRAY_BUFFER, BufferSize, MeshInternalData.Data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MeshInternalData.IndexCount * sizeof(i32), MeshInternalData.Indices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, POSITIONS_PER_VERTEX, GL_FLOAT, GL_FALSE,
                          POSITIONS_PER_VERTEX * sizeof(f32),
                          (void *) ((u8 *) MeshInternalData.Positions - MeshInternalData.Data));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, UVS_PER_VERTEX, GL_FLOAT, GL_FALSE,
                          UVS_PER_VERTEX * sizeof(f32),
                          (void *) ((u8 *) MeshInternalData.UVs - MeshInternalData.Data));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, NORMALS_PER_VERTEX, GL_FLOAT, GL_FALSE,
                          NORMALS_PER_VERTEX * sizeof(f32),
                          (void *) ((u8 *) MeshInternalData.Normals - MeshInternalData.Data));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, TANGENTS_PER_VERTEX, GL_FLOAT, GL_FALSE,
                          TANGENTS_PER_VERTEX * sizeof(f32),
                          (void *) ((u8 *) MeshInternalData.Tangents - MeshInternalData.Data));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, BITANGENTS_PER_VERTEX, GL_FLOAT, GL_FALSE,
                          BITANGENTS_PER_VERTEX * sizeof(f32),
                          (void *) ((u8 *) MeshInternalData.Bitangents - MeshInternalData.Data));
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, MAX_BONES_PER_VERTEX, GL_INT,
                           MAX_BONES_PER_VERTEX * sizeof(i32),
                           (void *) ((u8 *) MeshInternalData.BoneIDs - MeshInternalData.Data));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, MAX_BONES_PER_VERTEX, GL_FLOAT, GL_FALSE,
                          MAX_BONES_PER_VERTEX * sizeof(f32),
                          (void *) ((u8 *) MeshInternalData.BoneWeights - MeshInternalData.Data));

    glBindVertexArray(0);

    Out_Mesh->VAO = VAO;
    Out_Mesh->IndexCount = MeshInternalData.IndexCount;
}

static void
LoadTexturesForMesh(mesh *Mesh, const char *ModelPath, aiMaterial *AssimpMaterial)
{
    char ModelPathOnStack[MAX_PATH_LENGTH];
    strncpy_s(ModelPathOnStack, ModelPath, MAX_PATH_LENGTH - 1);
    i32 ModelPathCount = GetNullTerminatedStringLength(ModelPathOnStack);
    char ModelDirectory[MAX_PATH_LENGTH];
    i32 ModelDirectoryCount;
    GetFileDirectory(ModelPathOnStack, ModelPathCount, ModelDirectory, &ModelDirectoryCount, MAX_PATH_LENGTH);

    aiTextureType TextureTypes[] = { 
        aiTextureType_DIFFUSE, aiTextureType_SPECULAR,
        aiTextureType_EMISSIVE, aiTextureType_HEIGHT };

    char TextureFilename[MAX_FILENAME_LENGTH]; i32 TextureFilenameCount;
    char TexturePath[MAX_PATH_LENGTH];
    for (i32 TextureType = 0; TextureType < 4; ++TextureType)
    {
        ASSIMP_GetTextureFilename(AssimpMaterial, TextureTypes[TextureType],
                                  TextureFilename, &TextureFilenameCount, MAX_FILENAME_LENGTH);

        TexturePath[0] = '\0';

        if (TextureFilenameCount > 0)
        {
            CatStrings(ModelDirectory, ModelDirectoryCount,
                       TextureFilename, TextureFilenameCount,
                       TexturePath, MAX_PATH_LENGTH);
            Mesh->TextureIDs[TextureType] = LoadTexture(TexturePath);
        }
    }
}

static inline void
RenderMeshList(mesh *Meshes, i32 MeshCount)
{
    for (i32 MeshIndex = 0; MeshIndex < (i32) MeshCount; ++MeshIndex)
    {
        mesh *Mesh = &Meshes[MeshIndex];

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Mesh->DiffuseMapID);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Mesh->SpecularMapID);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, Mesh->EmissionMapID);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, Mesh->NormalMapID);

        glBindVertexArray(Mesh->VAO);
        glDrawElements(GL_TRIANGLES, Mesh->IndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

static inline void
UpdateAnimationState(animation *Animation, f32 DeltaTime)
{
    // Update current animation time to the beginning of the game loop
    Animation->CurrentTicks += DeltaTime * Animation->TicksPerSecond;

    // Loop animation around if past end
    if (Animation->CurrentTicks >= Animation->TicksDuration)
    {
        Animation->CurrentTicks -= Animation->TicksDuration;
    }
}

static inline i32
FindNextAnimationKey(animation *Animation)
{
    i32 Result = -1;

    for (i32 KeyIndex = 1; KeyIndex < Animation->KeyCount; KeyIndex++)
    {
        if (Animation->CurrentTicks <= Animation->KeyTimes[KeyIndex])
        {
            Result = KeyIndex;
            break;
        }
    }
    Assert(Result >= 1 && Result < Animation->KeyCount);

    return Result;
}

static inline f32 
CalculateLerpRatioBetweenTwoFrames(animation *Animation, i32 NextKey)
{
    f32 Result;

    Result = ((Animation->CurrentTicks - Animation->KeyTimes[NextKey-1]) /
              (Animation->KeyTimes[NextKey] - Animation->KeyTimes[NextKey-1]));

    return Result;
}

// TODO: Is it better to copy here, or deal with aliasing with 2 animation_key pointers?
static inline animation_key
LerpAnimationKeys(animation_key KeyA, animation_key KeyB, f32 LerpRatio)
{
    animation_key Result{ };

    Result.Position = KeyA.Position + LerpRatio * (KeyB.Position - KeyA.Position);
    Result.Rotation = glm::slerp(KeyA.Rotation, KeyB.Rotation, LerpRatio);
    Result.Scale = KeyA.Scale + LerpRatio * (KeyB.Scale - KeyA.Scale);

    return Result;
}

static inline animation_key
GetRestAnimationKeyForBone(bone Bone)
{
    animation_key Result{ };

    // NOTE: Decompose the bone's (node's) transform to parent (bone/node) into separate
    //       position, scaling and rotation components so they can be interpolated:
    //         - 4th column (only xyz) gives position
    //         - Lengths of first 3 columns (only xyz) gives scaling
    //         - 3x3 matrix, where each column is a unit vector (i.e. minus scale) is the rotation matrix;
    //           it will be cast to a quaternion
    
    Result.Position = Bone.TransformToParent[3];
    
    Result.Scale.x = glm::length(glm::vec3(Bone.TransformToParent[0]));
    Result.Scale.y = glm::length(glm::vec3(Bone.TransformToParent[1]));
    Result.Scale.z = glm::length(glm::vec3(Bone.TransformToParent[2]));
    
    glm::mat3 RotationMatrix{ };
    RotationMatrix[0] = glm::vec3(Bone.TransformToParent[0]) / Result.Scale.x;
    RotationMatrix[1] = glm::vec3(Bone.TransformToParent[1]) / Result.Scale.y;
    RotationMatrix[2] = glm::vec3(Bone.TransformToParent[2]) / Result.Scale.z;
    Result.Rotation = glm::quat_cast(RotationMatrix);

    return Result;
}

static inline glm::mat4
GetTransformationForAnimationKey(animation_key Key)
{
    glm::mat4 Result{ };

    glm::mat4 TranslateTransform = glm::translate(glm::mat4(1.0f), Key.Position);
    glm::mat4 RotateTransform = glm::mat4_cast(Key.Rotation);
    glm::mat4 ScaleTransform = glm::scale(glm::mat4(1.0f), Key.Scale);
    Result = TranslateTransform * RotateTransform * ScaleTransform;

    return Result;
}
