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

glm::mat4 ASSIMP_Mat4ToGLM(aiMatrix4x4 AssimpMat);
glm::vec3 ASSIMP_Vec3ToGLM(aiVector3D AssimpVector);
glm::quat ASSIMP_QuatToGLM(aiQuaternion AssimpQuat);
//u32 prepareMeshVAO(f32 *vertexData, u32 vertexCount, u32 *indices, u32 indexCount);
void PrepareSkinnedMeshRenderData(mesh_internal_data MeshInternalData, mesh *Out_Mesh);
mesh_internal_data InitializeMeshInternalData(i32 VertexCount, i32 IndexCount);
void FreeMeshInternalData(mesh_internal_data *MeshInternalData);
inline glm::mat4 GetTransformationForAnimationKey(animation_key Key);
inline animation_key LerpAnimationKeys(animation_key KeyA, animation_key KeyB, f32 LerpRatio);
inline void UpdateAnimationState(animation *Animation, f32 DeltaTime);
inline i32 FindNextAnimationKey(animation *Animation);
inline f32 CalculateLerpRatioBetweenTwoFrames(animation *Animation, i32 NextKey);
inline animation_key GetRestAnimationKeyForBone(bone Bone);

bone *ASSIMP_ParseBones(aiNode *ArmatureNode, i32 BoneCount)
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
            strncpy_s(Bone.Name, "DummyBone", MAX_ASSIMP_NAME_LENGTH -1);
        }
        else
        {
            strncpy_s(Bone.Name, CurrentNode->mName.C_Str(), MAX_ASSIMP_NAME_LENGTH -1);
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

// TODO: Now I want to get rid of recursion again...
void ASSIMP_GetArmatureInfoHelper(aiNode *Node, aiNode **Out_ArmatureNode, i32 *Out_BoneCount)
{
    std::cout << Node->mName.C_Str() << ": ";
    if (strcmp(Node->mName.C_Str(), "Armature") == 0)
    {
        *Out_ArmatureNode = Node;
    }
    else if (*Out_ArmatureNode)
    {
        std::cout << "++Out_BoneCount: ";
        ++(*Out_BoneCount);
    }

    std::cout << *Out_BoneCount << '\n';

    for (i32 Index = 0; Index < (i32) Node->mNumChildren; ++Index)
    {
        ASSIMP_GetArmatureInfoHelper(Node->mChildren[Index], Out_ArmatureNode, Out_BoneCount);
    }
}

void ASSIMP_GetArmatureInfo(aiNode *RootNode, aiNode **Out_ArmatureNode, i32 *Out_BoneCount)
{
    // Allocate for one extra bone, so the bone indexed 0 can count as no bone
    // TODO: Deal with assimp's "neutral_bone" for some models
    *Out_BoneCount = 1;
    *Out_ArmatureNode = 0;

    ASSIMP_GetArmatureInfoHelper(RootNode, Out_ArmatureNode, Out_BoneCount);
}

mesh_internal_data InitializeMeshInternalData(i32 VertexCount, i32 IndexCount)
{
    mesh_internal_data Result{ };

    size_t BytesToAllocate = VertexCount * (POSITIONS_PER_VERTEX * sizeof(f32) +
                                            UVS_PER_VERTEX * sizeof(f32) +
                                            NORMALS_PER_VERTEX * sizeof(f32) +
                                            TANGENTS_PER_VERTEX * sizeof(f32) +
                                            BITANGENTS_PER_VERTEX * sizeof(f32) +
                                            MAX_BONES_PER_VERTEX * sizeof(i32) +
                                            MAX_BONES_PER_VERTEX * sizeof(i32) +
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
        Result.BoneIDs = (i32 *) (Result.Bitangents + VertexCount * BITANGENTS_PER_VERTEX);
        Result.BoneWeights = (f32 *) (Result.BoneIDs + VertexCount * MAX_BONES_PER_VERTEX);
        Result.Indices = (i32 *) (Result.BoneWeights + VertexCount * MAX_BONES_PER_VERTEX);
    }
    else
    {
        // TODO: Logging
    }

    return Result;
}

void FreeMeshInternalData(mesh_internal_data *MeshInternalData)
{
    free(MeshInternalData->Data);
    memset(MeshInternalData, 0, sizeof(mesh_internal_data));
}

skinned_model LoadSkinnedModel(const char *Path)
{
    std::cout << "Loading model at: " << Path << '\n';

    // Result
    // ------
    skinned_model Model{ };

    // Load assimp scene
    // -----------------
    const aiScene *AssimpScene = aiImportFile(Path,
                                              aiProcess_CalcTangentSpace |
                                              aiProcess_Triangulate |
                                              aiProcess_JoinIdenticalVertices |
                                              aiProcess_FlipUVs);

    if (!AssimpScene || AssimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !AssimpScene->mRootNode)
    {
        // TODO: Logging
        std::cerr << "ERROR::LOAD_MODEL::ASSIMP_READ_ERROR: " << " Path: " << Path << '\n' << aiGetErrorString() << '\n';
        return Model;
    }

    // Scene armature data
    // -------------------
    aiNode *ArmatureNode = 0;
    i32 BoneCount = 0;
    ASSIMP_GetArmatureInfo(AssimpScene->mRootNode, &ArmatureNode, &BoneCount);
    if (ArmatureNode && BoneCount > 0)
    {
        Model.BoneCount = BoneCount;
        Model.Bones = ASSIMP_ParseBones(ArmatureNode, BoneCount);
    }

    // Scene mesh data
    // ---------------
    Model.MeshCount = AssimpScene->mNumMeshes;
    // TODO: LEAK
    Model.Meshes = (mesh *) calloc(1, Model.MeshCount * sizeof(mesh));
    if (!Model.Meshes)
    {
        std::cerr << "ERROR::LOAD_MODEL: Could not allocate space for meshes" << '\n';
        free(Model.Bones);
        Model.Bones = 0;
        return Model;
    }
    for (i32 MeshIndex = 0; MeshIndex < Model.MeshCount; ++MeshIndex)
    {
        aiMesh *AssimpMesh = AssimpScene->mMeshes[MeshIndex];

        i32 VertexCount = AssimpMesh->mNumVertices;
        i32 IndexCount = AssimpMesh->mNumFaces * 3;

        mesh_internal_data InternalData = InitializeMeshInternalData(VertexCount, IndexCount);

        // Mesh vertex data
        // ----------------
        for (i32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
        {
            for (i32 Index = 0; Index < POSITIONS_PER_VERTEX; ++Index)
            {
                InternalData.Positions[VertexIndex * POSITIONS_PER_VERTEX + Index] =
                    AssimpMesh->mVertices[VertexIndex][Index];
            }

            for (i32 Index = 0; Index < UVS_PER_VERTEX; ++Index)
            {
                InternalData.UVs[VertexIndex * UVS_PER_VERTEX + Index] =
                    AssimpMesh->mTextureCoords[0][VertexIndex][Index];
            }

            for (i32 Index = 0; Index < NORMALS_PER_VERTEX; ++Index)
            {
                InternalData.Normals[VertexIndex * NORMALS_PER_VERTEX + Index] =
                    AssimpMesh->mNormals[VertexIndex][Index];
            }

            for (i32 Index = 0; Index < TANGENTS_PER_VERTEX; ++Index)
            {
                InternalData.Tangents[VertexIndex * TANGENTS_PER_VERTEX + Index] =
                    AssimpMesh->mTangents[VertexIndex][Index];
            }

            for (i32 Index = 0; Index < BITANGENTS_PER_VERTEX; ++Index)
            {
                InternalData.Bitangents[VertexIndex * BITANGENTS_PER_VERTEX + Index] =
                    AssimpMesh->mBitangents[VertexIndex][Index];
            }
        }

        // Mesh index data
        // ---------------
        i32 *IndexCursor = InternalData.Indices;
        for (i32 FaceIndex = 0; FaceIndex < (i32) AssimpMesh->mNumFaces; ++FaceIndex)
        {
            aiFace *AssimpFace = &AssimpMesh->mFaces[FaceIndex];
            for (i32 Index = 0; Index < (i32) AssimpFace->mNumIndices; ++Index)
            {
                Assert(IndexCursor < InternalData.Indices + InternalData.IndexCount);

                *IndexCursor++ = AssimpFace->mIndices[Index];
            }
        }

        // Mesh bone data
        // --------------
        if (Model.Bones)
        {
            for (i32 AssimpBoneIndex = 0; AssimpBoneIndex < (i32) AssimpMesh->mNumBones; ++AssimpBoneIndex)
            {
                aiBone *AssimpBone = AssimpMesh->mBones[AssimpBoneIndex];

                i32 InternalBoneID = 0;

                for (i32 SearchBoneIndex = 0; SearchBoneIndex < BoneCount; ++SearchBoneIndex)
                {
                    if (strcmp(Model.Bones[SearchBoneIndex].Name, AssimpBone->mName.C_Str()) == 0)
                    {
                        InternalBoneID = SearchBoneIndex;
                    }
                }

                Model.Bones[InternalBoneID].InverseBindTransform = ASSIMP_Mat4ToGLM(AssimpBone->mOffsetMatrix);

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
                            if (InternalData.BoneIDs[VertexBonePosition] == 0)
                            {
                                InternalData.BoneIDs[VertexBonePosition] = InternalBoneID;
                                InternalData.BoneWeights[VertexBonePosition] = AssimpWeight->mWeight;
                                SpaceForBoneFound = true;
                                break;
                            }
                        }
                        Assert(SpaceForBoneFound);
                    }
                }
            }
        }

        mesh Mesh{ };
        PrepareSkinnedMeshRenderData(InternalData, &Mesh);

        FreeMeshInternalData(&InternalData);

        // Load textures
        // -------------
        // TODO

        // Save mesh into model
        // --------------------
        Model.Meshes[MeshIndex] = Mesh;
    }

    // Scene animation data
    // --------------------
    i32 AnimationCount = AssimpScene->mNumAnimations;
    Model.AnimationCount = AnimationCount;
    //TODO: LEAK
    Model.Animations = (animation *) calloc(1, AnimationCount * sizeof(animation));
    Assert(Model.Animations);

    // NOTE: An assumption I'm making: all animations for the same model have the same number of channels
    i32 ModelAnimationChannelCount = AssimpScene->mAnimations[0]->mNumChannels;

    for (i32 AnimationIndex = 0; AnimationIndex < AnimationCount; ++AnimationIndex)
    {
        aiAnimation *AssimpAnimation = AssimpScene->mAnimations[AnimationIndex];
        i32 KeyCount = 0;
        i32 ChannelCount = AssimpAnimation->mNumChannels;
        Assert(ChannelCount == ModelAnimationChannelCount);
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
            for (i32 BoneIndex = 0; BoneIndex < Model.BoneCount; ++BoneIndex)
            {
                if (strcmp(Model.Bones[BoneIndex].Name, AssimpAnimationChannel->mNodeName.C_Str()) == 0)
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

        strncpy_s(Model.Animations[AnimationIndex].Name, AssimpAnimation->mName.C_Str(), MAX_ASSIMP_NAME_LENGTH - 1);
        Model.Animations[AnimationIndex].TicksDuration = (f32) AssimpAnimation->mDuration;
        Model.Animations[AnimationIndex].TicksPerSecond = (f32) AssimpAnimation->mTicksPerSecond;
        Model.Animations[AnimationIndex].KeyCount = KeyCount;
        Model.Animations[AnimationIndex].ChannelCount = ChannelCount;
        Model.Animations[AnimationIndex].KeyTimes = AnimationKeyTimes;
        Model.Animations[AnimationIndex].Keys = AnimationKeys;
        // TODO: LEAK
        Model.AnimationState.TransientChannelTransformData = 
            (glm::mat4 *) calloc(1, Model.Animations[AnimationIndex].ChannelCount * sizeof(glm::mat4));
        Assert(Model.AnimationState.TransientChannelTransformData);
    }

    aiReleaseImport(AssimpScene);

    return Model;
}

#if 0
std::vector<Mesh> loadModel(const char *path)
{
    std::vector<Mesh> meshes;

    const aiScene *assimpScene = aiImportFile(path,
                                              aiProcess_CalcTangentSpace |
                                              aiProcess_Triangulate |
                                              aiProcess_JoinIdenticalVertices |
                                              aiProcess_FlipUVs);

    if (!assimpScene || assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !assimpScene->mRootNode)
    {
        std::cerr << "ERROR::LOAD_MODEL::ASSIMP_READ_ERROR: " << " path: " << path << '\n' << aiGetErrorString() << '\n';
        return meshes;
    }

    std::unordered_map<std::string, u32> loadedTextures{ };

    for (u32 meshIndex = 0; meshIndex < assimpScene->mNumMeshes; ++meshIndex)
    {
        std::cout << "Loading mesh #" << meshIndex << "/" << assimpScene->mNumMeshes << '\n';
        Mesh mesh;

        aiMesh *assimpMesh = assimpScene->mMeshes[meshIndex];

        f32 *meshVertexData;
        i32 meshVertexFloatCount;
        u32 *meshIndices;
        i32 meshIndexCount;
        // Position + Normal + UVs + Tangent + Bitangent
        i32 perVertexFloatCount = (3 + 3 + 2 + 3 + 3);
        u32 assimpVertexCount = assimpMesh->mNumVertices;
        meshVertexFloatCount = perVertexFloatCount * assimpVertexCount;
        meshVertexData = (f32 *) calloc(1, meshVertexFloatCount * sizeof(f32));
        if (!meshVertexData)
        {
            std::cerr << "ERROR::LOAD_MODEL::ALLOC_TEMP_HEAP_ERROR: path: " << path << '\n';
            return meshes;
        }
        for (u32 assimpVertexIndex = 0; assimpVertexIndex < assimpVertexCount; ++assimpVertexIndex)
        {
            f32 *meshVertexDataCursor = meshVertexData + (assimpVertexIndex * perVertexFloatCount);

            // Positions
            meshVertexDataCursor[0] = assimpMesh->mVertices[assimpVertexIndex].x;
            meshVertexDataCursor[1] = assimpMesh->mVertices[assimpVertexIndex].y;
            meshVertexDataCursor[2] = assimpMesh->mVertices[assimpVertexIndex].z;
            // Normals               
            meshVertexDataCursor[3] = assimpMesh->mNormals[assimpVertexIndex].x;
            meshVertexDataCursor[4] = assimpMesh->mNormals[assimpVertexIndex].y;
            meshVertexDataCursor[5] = assimpMesh->mNormals[assimpVertexIndex].z;
            // UVs                   
            meshVertexDataCursor[6] = assimpMesh->mTextureCoords[0][assimpVertexIndex].x;
            meshVertexDataCursor[7] = assimpMesh->mTextureCoords[0][assimpVertexIndex].y;
            // Tangent               
            meshVertexDataCursor[8] = assimpMesh->mTangents[assimpVertexIndex].x;
            meshVertexDataCursor[9] = assimpMesh->mTangents[assimpVertexIndex].y;
            meshVertexDataCursor[10] = assimpMesh->mTangents[assimpVertexIndex].z;
            // Bitangent
            meshVertexDataCursor[11] = assimpMesh->mBitangents[assimpVertexIndex].x;
            meshVertexDataCursor[12] = assimpMesh->mBitangents[assimpVertexIndex].y;
            meshVertexDataCursor[13] = assimpMesh->mBitangents[assimpVertexIndex].z;
        }
        // TODO: This assumes faces are triangles; might cause problems in the future
        u32 perFaceIndexCount = 3;
        u32 assimpFaceCount = assimpMesh->mNumFaces;
        meshIndexCount = assimpFaceCount * perFaceIndexCount;
        // TODO: Custom memory allocation
        meshIndices = (u32 *) calloc(1, meshIndexCount * sizeof(u32));
        if (!meshIndices)
        {
            std::cerr << "ERROR::LOAD_MODEL::ALLOC_TEMP_HEAP_ERROR: path: " << path << '\n';
            return meshes;
        }
        u32 *meshIndicesCursor = meshIndices;
        for (u32 assimpFaceIndex = 0; assimpFaceIndex < assimpFaceCount; ++assimpFaceIndex)
        {
            aiFace assimpFace = assimpMesh->mFaces[assimpFaceIndex];
            for (u32 assimpIndexIndex = 0; assimpIndexIndex < assimpFace.mNumIndices; ++assimpIndexIndex)
            {
                *meshIndicesCursor++ = assimpFace.mIndices[assimpIndexIndex];
            }
        }
        mesh.vao = prepareMeshVAO(meshVertexData, meshVertexFloatCount, meshIndices, meshIndexCount);
        mesh.indexCount = meshIndexCount;

        free(meshVertexData);
        free(meshIndices);

        // TODO: Refactor this
        aiMaterial *mat = assimpScene->mMaterials[assimpMesh->mMaterialIndex];
        u32 diffuseCount = aiGetMaterialTextureCount(mat, aiTextureType_DIFFUSE);
        if (diffuseCount > 0)
        {
            aiString assimpTextureFileName;
            aiGetMaterialString(mat, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), &assimpTextureFileName);

            std::string textureFileName = std::string(assimpTextureFileName.C_Str());
            if (loadedTextures.find(textureFileName) == loadedTextures.end())
            {
                std::string modelPath = std::string(path);
                std::string modelDirectory = modelPath.substr(0, modelPath.find_last_of('/'));
                std::string texturePath = modelDirectory + '/' + textureFileName;
                mesh.diffuseMapID = loadTexture(texturePath.c_str());
                loadedTextures[textureFileName] = mesh.diffuseMapID;
            }
            else
            {
                mesh.diffuseMapID = loadedTextures[textureFileName];
            }
        }
        u32 specularCount = aiGetMaterialTextureCount(mat, aiTextureType_SPECULAR);
        if (specularCount > 0)
        {
            aiString assimpTextureFileName;
            aiGetMaterialString(mat, AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), &assimpTextureFileName);

            std::string textureFileName = std::string(assimpTextureFileName.C_Str());
            if (loadedTextures.find(textureFileName) == loadedTextures.end())
            {
                std::string modelPath = std::string(path);
                std::string modelDirectory = modelPath.substr(0, modelPath.find_last_of('/'));
                std::string texturePath = modelDirectory + '/' + textureFileName;
                mesh.specularMapID = loadTexture(texturePath.c_str());
                loadedTextures[textureFileName] = mesh.specularMapID;
            }
            else
            {
                mesh.specularMapID = loadedTextures[textureFileName];
            }
        }
        u32 emissionCount = aiGetMaterialTextureCount(mat, aiTextureType_EMISSIVE);
        if (emissionCount > 0)
        {
            aiString assimpTextureFileName;
            aiGetMaterialString(mat, AI_MATKEY_TEXTURE(aiTextureType_EMISSIVE, 0), &assimpTextureFileName);

            std::string textureFileName = std::string(assimpTextureFileName.C_Str());
            if (loadedTextures.find(textureFileName) == loadedTextures.end())
            {
                std::string modelPath = std::string(path);
                std::string modelDirectory = modelPath.substr(0, modelPath.find_last_of('/'));
                std::string texturePath = modelDirectory + '/' + textureFileName;
                mesh.emissionMapID = loadTexture(texturePath.c_str());
                loadedTextures[textureFileName] = mesh.emissionMapID;
            }
            else
            {
                mesh.emissionMapID = loadedTextures[textureFileName];
            }
        }
        u32 normalCount = aiGetMaterialTextureCount(mat, aiTextureType_HEIGHT);
        if (normalCount > 0)
        {
            aiString assimpTextureFileName;
            aiGetMaterialString(mat, AI_MATKEY_TEXTURE(aiTextureType_HEIGHT, 0), &assimpTextureFileName);

            std::string textureFileName = std::string(assimpTextureFileName.C_Str());
            if (loadedTextures.find(textureFileName) == loadedTextures.end())
            {
                std::string modelPath = std::string(path);
                std::string modelDirectory = modelPath.substr(0, modelPath.find_last_of('/'));
                std::string texturePath = modelDirectory + '/' + textureFileName;
                mesh.normalMapID = loadTexture(texturePath.c_str());
                loadedTextures[textureFileName] = mesh.normalMapID;
            }
            else
            {
                mesh.normalMapID = loadedTextures[textureFileName];
            }
        }

        meshes.push_back(mesh);
    }

    aiReleaseImport(assimpScene);

    return meshes;
}
#endif

glm::mat4 ASSIMP_Mat4ToGLM(aiMatrix4x4 AssimpMat)
{
    glm::mat4 Result{ };

    Result[0][0] = AssimpMat.a1; Result[0][1] = AssimpMat.b1; Result[0][2] = AssimpMat.c1; Result[0][3] = AssimpMat.d1;
    Result[1][0] = AssimpMat.a2; Result[1][1] = AssimpMat.b2; Result[1][2] = AssimpMat.c2; Result[1][3] = AssimpMat.d2;
    Result[2][0] = AssimpMat.a3; Result[2][1] = AssimpMat.b3; Result[2][2] = AssimpMat.c3; Result[2][3] = AssimpMat.d3;
    Result[3][0] = AssimpMat.a4; Result[3][1] = AssimpMat.b4; Result[3][2] = AssimpMat.c4; Result[3][3] = AssimpMat.d4;

    return Result;
}

glm::vec3 ASSIMP_Vec3ToGLM(aiVector3D AssimpVector)
{
    glm::vec3 Result{ };

    Result.x = AssimpVector.x;
    Result.y = AssimpVector.y;
    Result.z = AssimpVector.z;

    return Result;
}

glm::quat ASSIMP_QuatToGLM(aiQuaternion AssimpQuat)
{
    glm::quat Result{ };

    Result.w = AssimpQuat.w;
    Result.x = AssimpQuat.x;
    Result.y = AssimpQuat.y;
    Result.z = AssimpQuat.z;

    return Result;
}

#if 0
u32 prepareMeshVAO(f32 *vertexData, u32 vertexAttribCount, u32 *indices, u32 indexCount)
{
    u32 meshVAO;
    glGenVertexArrays(1, &meshVAO);
    u32 meshVBO;
    glGenBuffers(1, &meshVBO);
    u32 meshEBO;
    glGenBuffers(1, &meshEBO);

    glBindVertexArray(meshVAO);
    glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexAttribCount * sizeof(f32), vertexData, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(u32), indices, GL_STATIC_DRAW);

    // Position + Normal + UVs + Tangent + Bitangent
    i32 vertexSize = (3 + 3 + 2 + 3 + 3) * sizeof(f32);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void *) 0);
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexSize, (void *) (3 * sizeof(f32)));
    // UVs
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexSize, (void *) (6 * sizeof(f32)));
    // Tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, vertexSize, (void *) (8 * sizeof(f32)));
    // Bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, vertexSize, (void *) (11 * sizeof(f32)));

    glBindVertexArray(0);

    return meshVAO;
}
#endif

void PrepareSkinnedMeshRenderData(mesh_internal_data MeshInternalData, mesh *Out_Mesh)
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

void Render(skinned_model *Model, u32 Shader, f32 DeltaTime)
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
        sprintf_s(LocationStringBuffer, "boneTransforms[%d]", BoneIndex);
        //Transform = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(Shader, LocationStringBuffer), 1, GL_FALSE, glm::value_ptr(Transform));
    }

    // Render each mesh of the model
    // -----------------------------
    for (i32 MeshIndex = 0; MeshIndex < (i32) Model->MeshCount; ++MeshIndex)
    {
        mesh Mesh = Model->Meshes[MeshIndex];
        glBindVertexArray(Mesh.VAO);
        glDrawElements(GL_TRIANGLES, Mesh.IndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    glUseProgram(0);
}

inline animation_key GetRestAnimationKeyForBone(bone Bone)
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

inline glm::mat4 GetTransformationForAnimationKey(animation_key Key)
{
    glm::mat4 Result{ };

    glm::mat4 TranslateTransform = glm::translate(glm::mat4(1.0f), Key.Position);
    glm::mat4 RotateTransform = glm::mat4_cast(Key.Rotation);
    glm::mat4 ScaleTransform = glm::scale(glm::mat4(1.0f), Key.Scale);
    Result = TranslateTransform * RotateTransform * ScaleTransform;

    return Result;
}

// TODO: Is it better to copy here, or deal with aliasing with 2 animation_key pointers?
inline animation_key LerpAnimationKeys(animation_key KeyA, animation_key KeyB, f32 LerpRatio)
{
    animation_key Result{ };
    
    Result.Position = KeyA.Position + LerpRatio * (KeyB.Position - KeyA.Position);
    Result.Rotation = glm::slerp(KeyA.Rotation, KeyB.Rotation, LerpRatio);
    Result.Scale = KeyA.Scale + LerpRatio * (KeyB.Scale - KeyA.Scale);

    return Result;
}

inline void UpdateAnimationState(animation *Animation, f32 DeltaTime)
{
    // Update current animation time to the beginning of the game loop
    Animation->CurrentTicks += DeltaTime * Animation->TicksPerSecond;

    // Loop animation around if past end
    if (Animation->CurrentTicks >= Animation->TicksDuration)
    {
        Animation->CurrentTicks -= Animation->TicksDuration;
    }
}

inline i32 FindNextAnimationKey(animation *Animation)
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

inline f32 CalculateLerpRatioBetweenTwoFrames(animation *Animation, i32 NextKey)
{
    f32 Result;

    Result = ((Animation->CurrentTicks - Animation->KeyTimes[NextKey-1]) /
              (Animation->KeyTimes[NextKey] - Animation->KeyTimes[NextKey-1]));

    return Result;
}
