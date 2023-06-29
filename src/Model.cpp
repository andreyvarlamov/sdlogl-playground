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

glm::mat4 ASSIMP_Mat4ToGLM(aiMatrix4x4 assimpMat);
u32 prepareMeshVAO(f32 *vertexData, u32 vertexCount, u32 *indices, u32 indexCount);
u32 prepareSkinnedMeshVAO(f32 *vertexData, u32 vertexCount, u32 *indices, u32 indexCount);

void ASSIMP_ParseBonesHelper(aiNode *Node, bone **Bones, i32 *CurrentIndex, i32 BoneCount)
{
    Assert(*CurrentIndex < BoneCount + 1);

    i32 ParentIndex = *CurrentIndex;

    for (i32 Index = 0; Index < (i32)Node->mNumChildren; ++Index)
    {
        aiNode *ChildNode = Node->mChildren[Index];
        ++(*CurrentIndex);

        bone ChildBone{ };
        strncpy_s(ChildBone.Name, ChildNode->mName.C_Str(), MAX_BONE_NAME_LENGTH - 1);
        ChildBone.ID = *CurrentIndex;
        ChildBone.ParentID = ParentIndex;
        ChildBone.TransformToParent = ASSIMP_Mat4ToGLM(ChildNode->mTransformation);
        (*Bones)[*CurrentIndex] = ChildBone;
        ASSIMP_ParseBonesHelper(ChildNode, Bones, CurrentIndex, BoneCount);
    }
}

void ASSIMP_ParseBones(aiNode *ArmatureNode, bone **Bones, i32 BoneCount)
{
    Assert(*Bones);
    Assert(BoneCount > 0);

    i32 BoneIndex = 0;

    bone DummyBone{ };
    strncpy_s(DummyBone.Name, "DummyBone", MAX_BONE_NAME_LENGTH - 1);
    (*Bones)[BoneIndex] = DummyBone;

    ASSIMP_ParseBonesHelper(ArmatureNode, Bones, &BoneIndex, BoneCount);
}

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

    for (i32 Index = 0; Index < (i32)Node->mNumChildren; ++Index)
    {
        ASSIMP_GetArmatureInfoHelper(Node->mChildren[Index], Out_ArmatureNode, Out_BoneCount);
    }
}

void ASSIMP_GetArmatureInfo(aiNode *RootNode, aiNode **Out_ArmatureNode, i32 *Out_BoneCount)
{
    *Out_BoneCount = 0;
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

    u8 *Data = (u8 *)calloc(1, BytesToAllocate);

    if (Data)
    {
        Result.Data = Data;
        Result.VertexCount = VertexCount;
        Result.IndexCount = IndexCount;
        Result.Positions = (f32 *)(Data);
        Result.UVs = (f32 *)(Result.Positions + VertexCount * POSITIONS_PER_VERTEX);
        Result.Normals = (f32 *)(Result.UVs + VertexCount * UVS_PER_VERTEX);
        Result.Tangents = (f32 *)(Result.Normals + VertexCount * NORMALS_PER_VERTEX);
        Result.Bitangents = (f32 *)(Result.Tangents + VertexCount * TANGENTS_PER_VERTEX);
        Result.BoneIDs = (i32 *)(Result.Bitangents + VertexCount * BITANGENTS_PER_VERTEX);
        Result.BoneWeights = (f32 *)(Result.BoneIDs + VertexCount * MAX_BONES_PER_VERTEX);
        Result.Indices = (i32 *)(Result.BoneWeights + VertexCount * MAX_BONES_PER_VERTEX);
    }
    else
    {
        // TODO: Logging
    }

    return Result;
}

void FreeMeshInternalData(mesh_internal_data *MeshData)
{
    free(MeshData->Data);
    memset(MeshData, 0, sizeof(mesh_internal_data));
}

skinned_model LoadSkinnedModel(const char *Path)
{
    std::cout << "Loading model at: " << Path << '\n';

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

    // Parse node structure: find armature node and count the number of bones
    aiNode *ArmatureNode = 0;
    i32 BoneCount;
    bone *Bones = 0;
    ASSIMP_GetArmatureInfo(AssimpScene->mRootNode, &ArmatureNode, &BoneCount);

    // Assimp node tree structure -> custom bone tree structure
    if (ArmatureNode)
    {
        Assert(BoneCount > 0);

        // Allocate for one extra bone, so the bone indexed 0 can count as no bone
        // TODO: Deal with assimp's "neutral_bone" for some models
        // TODO: LEAK
        Bones = (bone *)calloc(1, (BoneCount + 1) * sizeof(bone));

        if (Bones)
        {
            std::cout << BoneCount * sizeof(bone) << " bytes allocated for bones array" << "\n";

            ASSIMP_ParseBones(ArmatureNode, &Bones, BoneCount);
        }
        else
        {
            // TODO: Logging
        }
    }

    i32 MeshCount = AssimpScene->mNumMeshes;

    // Parse meshes
    for (i32 MeshIndex = 0; MeshIndex < MeshCount; ++MeshIndex)
    {
        aiMesh *AssimpMesh = AssimpScene->mMeshes[MeshIndex];

        i32 VertexCount = AssimpMesh->mNumVertices;
        i32 IndexCount = AssimpMesh->mNumFaces * 3;

        mesh_internal_data InternalData = InitializeMeshInternalData(VertexCount, IndexCount);

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

        for (i32 FaceIndex = 0; FaceIndex < (i32) AssimpMesh->mNumFaces; ++FaceIndex)
        {
            aiFace *AssimpFace = &AssimpMesh->mFaces[FaceIndex];
            i32 *IndexCursor = InternalData.Indices;
            for (i32 Index = 0; Index < (i32) AssimpFace->mNumIndices; ++Index)
            {
                Assert(IndexCursor < InternalData.Indices + InternalData.IndexCount);

                *IndexCursor++ = AssimpFace->mIndices[Index];
            }
        }

        if (Bones)
        {
            for (i32 AssimpBoneIndex = 0; AssimpBoneIndex < (i32) AssimpMesh->mNumBones; ++AssimpBoneIndex)
            {
                aiBone *AssimpBone = AssimpMesh->mBones[AssimpBoneIndex];

                i32 InternalBoneID = 0;

                for (i32 SearchBoneIndex = 0; SearchBoneIndex < BoneCount; ++SearchBoneIndex)
                {
                    if (strcmp(Bones[SearchBoneIndex].Name, AssimpBone->mName.C_Str()) == 0)
                    {
                        InternalBoneID = SearchBoneIndex;
                    }
                }

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

        FreeMeshInternalData(&InternalData);

        std::cout << ".\n";
    }

#if 0
    // Parse bone data
    // ---------------
    // 1. Find armature node
    std::cout << "Looking for armature node... ";
    aiNode *armatureNode = 0;
    bool hasArmature = false;
    std::queue<aiNode *> nodeQueue;
    nodeQueue.push(AssimpScene->mRootNode);
    while (!nodeQueue.empty() && !hasArmature)
    {
        aiNode *frontNode = nodeQueue.front();
        nodeQueue.pop();

        if (strcmp(frontNode->mName.C_Str(), "Armature") == 0)
        {
            armatureNode = frontNode;
            hasArmature = true;
        }

        for (i32 i = 0; i < (i32)frontNode->mNumChildren; ++i)
        {
            nodeQueue.push(frontNode->mChildren[i]);
        }
    }

    // 2. Get bone tree and transform to parent data
    if (hasArmature)
    {
        i32 bonesParsed = 0;
        std::vector<aiNode *> nodes{ }; // Temp aiNode storage in the same order as bones 
        std::stack<i32> tempNodeStack{ }; // Temp tree-DFS-stack that stores bone IDs for 'bones' and 'nodes' vectors
        bool isFirstIteration = true;
        while (isFirstIteration || !tempNodeStack.empty())
        {
            if (isFirstIteration)
            {

                for (i32 i = 0; i < (i32)armatureNode->mNumChildren; ++i)
                {
                    aiNode *rootBoneNode = armatureNode->mChildren[i];
                    nodes.push_back(rootBoneNode);

                    Bone rootBone{ };
                    rootBone.name = rootBoneNode->mName.C_Str();
                    rootBone.transformToParent = ASSIMP_Mat4ToGLM(rootBoneNode->mTransformation);
                    Model.bones.push_back(rootBone);

                    tempNodeStack.push(bonesParsed); // bonesParsed acting as ID of the bone that was just added
                    ++bonesParsed;
                }

                isFirstIteration = false;
            }
            else
            {
                i32 parentIndex = tempNodeStack.top();
                tempNodeStack.pop();

                aiNode *parentNode = nodes[parentIndex];
                Bone *parentBone = &Model.bones[parentIndex];

                for (i32 i = 0; i < (i32)parentNode->mNumChildren; ++i)
                {
                    aiNode *childNode = parentNode->mChildren[i];
                    nodes.push_back(childNode);

                    Bone childBone{ };
                    childBone.name = childNode->mName.C_Str();
                    memcpy(childBone.pathToRoot, parentBone->pathToRoot, parentBone->pathNodes * sizeof(i32));
                    childBone.pathNodes = parentBone->pathNodes;
                    childBone.pathToRoot[childBone.pathNodes] = parentIndex;
                    ++childBone.pathNodes;
                    childBone.transformToParent = ASSIMP_Mat4ToGLM(childNode->mTransformation);
                    Model.bones.push_back(childBone);
                    // reassign because push back could've invalidated that pointer
                    parentBone = &Model.bones[parentIndex];

                    tempNodeStack.push(bonesParsed);
                    ++bonesParsed;
                }
            }
        }

        std::cout << "Armature found. " << Model.bones.size() << " bones:\n";

        for (i32 i = 0; i < Model.bones.size(); ++i)
        {
            std::cout << "Bone #" << i << "/" << Model.bones.size() << ": " << Model.bones[i].name << ". Path to root: ";

            for (i32 pathIndex = 0; pathIndex < Model.bones[i].pathNodes; ++pathIndex)
            {
                std::cout << Model.bones[i].pathToRoot[pathIndex];

                if (pathIndex != Model.bones[i].pathNodes - 1)
                {
                    std::cout << ", ";
                }
            }

            std::cout << '\n';
        }
    }
    else
    {
        std::cout << "Not found.\n";
    }

    std::cout << '\n';

    for (i32 meshIndex = 0; meshIndex < (i32)assimpScene->mNumMeshes; ++meshIndex)
    {
        aiMesh *assimpMesh = assimpScene->mMeshes[meshIndex];
        std::cout << "Loading mesh #" << meshIndex << "/" << assimpScene->mNumMeshes << " " << assimpMesh->mName.C_Str() << '\n';

        i32 vertexCount = assimpMesh->mNumVertices;
        i32 faceCount = assimpMesh->mNumFaces;
        i32 indexCount = assimpMesh->mNumFaces * 3;
        i32 boneCount = assimpMesh->mNumBones;
        std::cout << " Vertices: " << vertexCount << " Faces: " << faceCount << " Indices: " << indexCount << " Bones: " << boneCount << '\n';

        struct VertexBoneInfo
        {
            i32 boneID[4];
            f32 boneWeight[4];
            i32 nextUnusedSlot;
        };

        VertexBoneInfo *vertexBones = (VertexBoneInfo *)calloc(1, vertexCount * sizeof(VertexBoneInfo));

        //std::cout << "\n  Bones:\n";
        for (i32 boneIndex = 0; boneIndex < boneCount; ++boneIndex)
        {
            aiBone *assimpBone = assimpMesh->mBones[boneIndex];
            i32 boneID;
            for (i32 i = 0; i < Model.bones.size(); ++i)
            {
                if (strcmp(Model.bones[i].name.c_str(), assimpBone->mName.C_Str()) == 0)
                {
                    boneID = i + 1;
                    break;
                }
            }
            //std::cout << "  Bone #" << boneIndex + 1 << "/" << boneCount << " " << assimpBone->mName.C_Str() << " -> BoneID: " << boneID << '\n';

            i32 weightCount = assimpBone->mNumWeights;
            //std::cout << "   Vertices affected: " << weightCount << '\n';

            for (i32 weightIndex = 0; weightIndex < weightCount; ++weightIndex)
            {
                aiVertexWeight assimpVertexWeight = assimpBone->mWeights[weightIndex];
                if (assimpVertexWeight.mWeight > 0.0f)
                {
                    //std::cout << "   Weight #" << weightIndex << "/" << weightCount << " vertex ID: " << assimpVertexWeight.mVertexId << " weight: " << assimpVertexWeight.mWeight << '\n';

                    VertexBoneInfo *vertexBone = &vertexBones[assimpVertexWeight.mVertexId];
                    if (vertexBone->nextUnusedSlot < 4)
                    {
                        vertexBone->boneID[vertexBone->nextUnusedSlot] = boneID;
                        vertexBone->boneWeight[vertexBone->nextUnusedSlot++] = assimpVertexWeight.mWeight;
                    }
                    else
                    {
                        std::cerr << "ERROR::LOAD_GLTF::MAX_BONE_PER_VERTEX_EXCEEDED" << '\n';
                    }

                }
            }

            Model.bones[boneID - 1].inverseBindTransform = ASSIMP_Mat4ToGLM(assimpBone->mOffsetMatrix);
        }

        // Position + 4 bone ids + 4 bone weights
        f32 *meshVertexData = (f32 *)calloc(1, 11 * vertexCount * sizeof(f32));

        //std::cout << "\n  Vertices:\n";
        for (i32 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
        {
            aiVector3D vertex = assimpMesh->mVertices[vertexIndex];
            //std::cout << "  Vertex #" << vertexIndex << "/" << vertexCount << " " << vertex.x << ", " << vertex.y << ", " << vertex.z << '\n';

            f32 *meshVertexDataCursor = meshVertexData + (vertexIndex * 11);

            meshVertexDataCursor[0] = vertex.x;
            meshVertexDataCursor[1] = vertex.y;
            meshVertexDataCursor[2] = vertex.z;

            meshVertexDataCursor[3] = *((f32 *)&vertexBones[vertexIndex].boneID[0]);
            meshVertexDataCursor[4] = *((f32 *)&vertexBones[vertexIndex].boneID[1]);
            meshVertexDataCursor[5] = *((f32 *)&vertexBones[vertexIndex].boneID[2]);
            meshVertexDataCursor[6] = *((f32 *)&vertexBones[vertexIndex].boneID[3]);

            meshVertexDataCursor[7] = vertexBones[vertexIndex].boneWeight[0];
            meshVertexDataCursor[8] = vertexBones[vertexIndex].boneWeight[1];
            meshVertexDataCursor[9] = vertexBones[vertexIndex].boneWeight[2];
            meshVertexDataCursor[10] = vertexBones[vertexIndex].boneWeight[3];
        }

        u32 *meshIndices = (u32 *)calloc(1, indexCount * sizeof(u32));

        //std::cout << "\n  Faces:\n";
        u32 *meshIndicesCursor = meshIndices;
        for (i32 faceIndex = 0; faceIndex < faceCount; ++faceIndex)
        {
            aiFace assimpFace = assimpMesh->mFaces[faceIndex];
            //std::cout << "  Face #" << faceIndex << "/" << faceCount << " Indices: " << assimpFace.mIndices[0] << ", " << assimpFace.mIndices[1] << ", " << assimpFace.mIndices[2] << '\n';

            for (u32 assimpIndexIndex = 0; assimpIndexIndex < assimpFace.mNumIndices; ++assimpIndexIndex)
            {
                *meshIndicesCursor++ = assimpFace.mIndices[assimpIndexIndex];
            }
        }

        Mesh mesh;
        mesh.vao = prepareSkinnedMeshVAO(meshVertexData, 11 * vertexCount, meshIndices, indexCount);
        mesh.indexCount = indexCount;

        Model.meshes.push_back(mesh);
    }

    if (assimpScene->mNumAnimations > 0)
    {
        aiAnimation *animation = assimpScene->mAnimations[0];
        i32 channelCount = (i32)animation->mNumChannels;
        std::cout << "\nFound animation: " << animation->mName.C_Str() << ". " << channelCount << " channels" << '\n';

        Model.animation.ticksDuration = (f32)animation->mDuration;
        Model.animation.ticksPerSecond = (f32)animation->mTicksPerSecond;

        for (i32 channelIndex = 0; channelIndex < channelCount; ++channelIndex)
        {
            aiNodeAnim *channel = animation->mChannels[channelIndex];

            Bone *Bone = 0;
            i32 boneID = 0;

            for (i32 boneIndex = 0; boneIndex < Model.bones.size(); ++boneIndex)
            {
                if (strcmp(Model.bones[boneIndex].name.c_str(), channel->mNodeName.C_Str()) == 0)
                {
                    Bone = &Model.bones[boneIndex];
                    boneID = boneIndex + 1;
                }
            }

            if (Bone)
            {
                i32 positionKeyCount = channel->mNumPositionKeys;
                i32 scalingKeyCount = channel->mNumScalingKeys;
                i32 rotationKeyCount = channel->mNumRotationKeys;

                std::cout << "  Channel #" << channelIndex << "/" << channelCount
                    << " for bone " << Bone->name << "(" << boneID << "). "
                    << "Position keys: " << positionKeyCount
                    << "; scaling keys: " << scalingKeyCount
                    << "; rotation keys: " << rotationKeyCount << '\n';

                Bone->positionKeys.resize(positionKeyCount);
                for (i32 i = 0; i < positionKeyCount; ++i)
                {
                    aiVectorKey *assimpPos = &channel->mPositionKeys[i];
                    PositionKey positionKey{ };
                    positionKey.time = (f32)assimpPos->mTime;
                    positionKey.position = glm::vec3(assimpPos->mValue.x, assimpPos->mValue.y, assimpPos->mValue.z);
                    Bone->positionKeys[i] = positionKey;
                    }

                Bone->scalingKeys.resize(scalingKeyCount);
                for (i32 i = 0; i < scalingKeyCount; ++i)
                {
                    aiVectorKey *assimpScale = &channel->mScalingKeys[i];
                    ScalingKey scalingKey{ };
                    scalingKey.time = (f32)assimpScale->mTime;
                    scalingKey.scale = glm::vec3(assimpScale->mValue.x, assimpScale->mValue.y, assimpScale->mValue.z);
                    Bone->scalingKeys[i] = scalingKey;
                }

                Bone->rotationKeys.resize(rotationKeyCount);
                for (i32 i = 0; i < rotationKeyCount; ++i)
                {
                    aiQuatKey *assimpRotation = &channel->mRotationKeys[i];
                    RotationKey rotationKey{ };
                    rotationKey.time = (f32)assimpRotation->mTime;
                    rotationKey.rotation = glm::quat(assimpRotation->mValue.w, assimpRotation->mValue.x, assimpRotation->mValue.y, assimpRotation->mValue.z);
                    Bone->rotationKeys[i] = rotationKey;
                }
            }
        }
    }
#endif

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
        meshVertexData = (f32 *)calloc(1, meshVertexFloatCount * sizeof(f32));
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
        meshIndices = (u32 *)calloc(1, meshIndexCount * sizeof(u32));
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

glm::mat4 ASSIMP_Mat4ToGLM(aiMatrix4x4 assimpMat)
{
    glm::mat4 result{ };

    result[0][0] = assimpMat.a1; result[0][1] = assimpMat.b1; result[0][2] = assimpMat.c1; result[0][3] = assimpMat.d1;
    result[1][0] = assimpMat.a2; result[1][1] = assimpMat.b2; result[1][2] = assimpMat.c2; result[1][3] = assimpMat.d2;
    result[2][0] = assimpMat.a3; result[2][1] = assimpMat.b3; result[2][2] = assimpMat.c3; result[2][3] = assimpMat.d3;
    result[3][0] = assimpMat.a4; result[3][1] = assimpMat.b4; result[3][2] = assimpMat.c4; result[3][3] = assimpMat.d4;

    return result;
}

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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void *)0);
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexSize, (void *)(3 * sizeof(f32)));
    // UVs
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexSize, (void *)(6 * sizeof(f32)));
    // Tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, vertexSize, (void *)(8 * sizeof(f32)));
    // Bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, vertexSize, (void *)(11 * sizeof(f32)));

    glBindVertexArray(0);

    return meshVAO;
}

u32 prepareSkinnedMeshVAO(f32 *vertexData, u32 vertexAttribCount, u32 *indices, u32 indexCount)
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

    // Position + 4 bone ids + 4 bone weights
    i32 vertexSize = 11 * sizeof(f32);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void *)0);
    // Bone IDs
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 4, GL_INT, vertexSize, (void *)(3 * sizeof(f32)));
    // Bone weights
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, vertexSize, (void *)(7 * sizeof(f32)));

    glBindVertexArray(0);

    return meshVAO;
}

void Render(skinned_model *Model, u32 Shader, f32 DeltaTime)
{
    glUseProgram(Shader);

#if 0
    if (Model->Animation.isRunning)
    {
        if (!Model->Animation.isPaused)
        {
            Model->Animation.currentTicks += DeltaTime * Model->Animation.ticksPerSecond;
            if (Model->Animation.currentTicks >= Model->Animation.ticksDuration)
            {
                if (Model->Animation.isLooped)
                {
                    Model->Animation.currentTicks -= Model->Animation.ticksDuration;
                }
                else
                {
                    Model->Animation.currentTicks = 0.0f;
                    Model->Animation.isRunning = false;
                }
            }
        }

        for (i32 atlbetaBoneIndex = 0; atlbetaBoneIndex < (i32)Model->Bones.size(); ++atlbetaBoneIndex)
        {
            bone Bone = Model->Bones[atlbetaBoneIndex];

            glm::mat4 boneTransform = Bone.InverseBindTransform;

            //boneTransform = bone.transformToParent * boneTransform;

            glm::mat4 scalingTransform(1.0f);
            glm::mat4 rotationTransform(1.0f);
            glm::mat4 translationTransform(1.0f);

            for (i32 i = 0; i < (i32)Bone.positionKeys.size() - 1; ++i)
            {
                if (Model->Animation.currentTicks >= Bone.positionKeys[i].time &&
                    Model->Animation.currentTicks < Bone.positionKeys[i + 1].time)
                {
                    f32 timeBetweenFrames = Bone.positionKeys[i + 1].time - Bone.positionKeys[i].time;
                    f32 timeAfterPreviousFrame = Model->Animation.currentTicks - Bone.positionKeys[i].time;
                    f32 percentNextFrame = timeAfterPreviousFrame / timeBetweenFrames;

                    glm::vec3 interpolatedPosition =
                        (Bone.positionKeys[i].position * (1 - percentNextFrame) +
                         Bone.positionKeys[i + 1].position * percentNextFrame);

                    translationTransform = glm::translate(glm::mat4(1.0f), interpolatedPosition);
                    break;
                }
            }
            for (i32 i = 0; i < (i32)Bone.rotationKeys.size() - 1; ++i)
            {
                if (Model->Animation.currentTicks >= Bone.rotationKeys[i].time &&
                    Model->Animation.currentTicks < Bone.rotationKeys[i + 1].time)
                {
                    f32 timeBetweenFrames = Bone.rotationKeys[i + 1].time - Bone.rotationKeys[i].time;
                    f32 timeAfterPreviousFrame = Model->Animation.currentTicks - Bone.rotationKeys[i].time;
                    f32 percentNextFrame = timeAfterPreviousFrame / timeBetweenFrames;

                    glm::quat interpolatedRotation = glm::slerp(Bone.rotationKeys[i].rotation,
                                                                Bone.rotationKeys[i + 1].rotation,
                                                                percentNextFrame);

                    rotationTransform = glm::mat4_cast(interpolatedRotation);
                    break;
                }
            }
            for (i32 i = 0; i < (i32)Bone.scalingKeys.size() - 1; ++i)
            {
                if (Model->Animation.currentTicks >= Bone.scalingKeys[i].time &&
                    Model->Animation.currentTicks < Bone.scalingKeys[i + 1].time)
                {
                    f32 timeBetweenFrames = Bone.scalingKeys[i + 1].time - Bone.scalingKeys[i].time;
                    f32 timeAfterPreviousFrame = Model->Animation.currentTicks - Bone.scalingKeys[i].time;
                    f32 percentNextFrame = timeAfterPreviousFrame / timeBetweenFrames;

                    glm::vec3 scale =
                        (Bone.scalingKeys[i].scale * (1 - percentNextFrame) +
                         Bone.scalingKeys[i + 1].scale * percentNextFrame);

                    scalingTransform = glm::scale(glm::mat4(1.0f), scale);
                    break;
                }
            }

            boneTransform = translationTransform * rotationTransform * scalingTransform * boneTransform;

            for (i32 parentIndex = Bone.pathNodes - 1; parentIndex >= 0; --parentIndex)
            {
                Bone parentBone = Model->Bones[Bone.pathToRoot[parentIndex]];
                //boneTransform = parentBone.transformToParent * boneTransform;

                glm::mat4 parentScalingTransform(1.0f);
                glm::mat4 parentRotationTransform(1.0f);
                glm::mat4 parentTranslationTransform(1.0f);

                for (i32 i = 0; i < (i32)parentBone.positionKeys.size() - 1; ++i)
                {
                    if (Model->Animation.currentTicks >= parentBone.positionKeys[i].time &&
                        Model->Animation.currentTicks < parentBone.positionKeys[i + 1].time)
                    {
                        f32 timeBetweenFrames = parentBone.positionKeys[i + 1].time - parentBone.positionKeys[i].time;
                        f32 timeAfterPreviousFrame = Model->Animation.currentTicks - parentBone.positionKeys[i].time;
                        f32 percentNextFrame = timeAfterPreviousFrame / timeBetweenFrames;

                        glm::vec3 interpolatedPosition =
                            (parentBone.positionKeys[i].position * (1 - percentNextFrame) +
                             parentBone.positionKeys[i + 1].position * percentNextFrame);

                        parentTranslationTransform = glm::translate(glm::mat4(1.0f), interpolatedPosition);
                        break;
                    }
                }
                for (i32 i = 0; i < (i32)parentBone.rotationKeys.size() - 1; ++i)
                {
                    if (Model->Animation.currentTicks >= parentBone.rotationKeys[i].time &&
                        Model->Animation.currentTicks < parentBone.rotationKeys[i + 1].time)
                    {
                        f32 timeBetweenFrames = parentBone.rotationKeys[i + 1].time - parentBone.rotationKeys[i].time;
                        f32 timeAfterPreviousFrame = Model->Animation.currentTicks - parentBone.rotationKeys[i].time;
                        f32 percentNextFrame = timeAfterPreviousFrame / timeBetweenFrames;

                        glm::quat interpolatedRotation = glm::slerp(parentBone.rotationKeys[i].rotation,
                                                                    parentBone.rotationKeys[i + 1].rotation,
                                                                    percentNextFrame);

                        parentRotationTransform = glm::mat4_cast(interpolatedRotation);
                        break;
                    }
                }
                for (i32 i = 0; i < (i32)parentBone.scalingKeys.size() - 1; ++i)
                {
                    if (Model->Animation.currentTicks >= parentBone.scalingKeys[i].time &&
                        Model->Animation.currentTicks < parentBone.scalingKeys[i + 1].time)
                    {
                        f32 timeBetweenFrames = parentBone.scalingKeys[i + 1].time - parentBone.scalingKeys[i].time;
                        f32 timeAfterPreviousFrame = Model->Animation.currentTicks - parentBone.scalingKeys[i].time;
                        f32 percentNextFrame = timeAfterPreviousFrame / timeBetweenFrames;

                        glm::vec3 scale =
                            (parentBone.scalingKeys[i].scale * (1 - percentNextFrame) +
                             parentBone.scalingKeys[i + 1].scale * percentNextFrame);

                        parentScalingTransform = glm::scale(glm::mat4(1.0f), scale);
                        break;
}
        }

                boneTransform = parentTranslationTransform * parentRotationTransform * parentScalingTransform * boneTransform;
    }

            std::string location = "boneTransforms[" + std::to_string(atlbetaBoneIndex) + "]";

            glUniformMatrix4fv(glGetUniformLocation(Shader, location.c_str()), 1, GL_FALSE, glm::value_ptr(boneTransform));
}
    }
    else
    {
        for (i32 atlbetaBoneIndex = 0; atlbetaBoneIndex < (i32)Model->Bones.size(); ++atlbetaBoneIndex)
        {
            std::string location = "boneTransforms[" + std::to_string(atlbetaBoneIndex) + "]";
            glm::mat4 boneTransform(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(Shader, location.c_str()), 1, GL_FALSE, glm::value_ptr(boneTransform));
        }
    }
#endif

    for (i32 MeshIndex = 0; MeshIndex < (i32)Model->Meshes.size(); ++MeshIndex)
    {
        mesh Mesh = Model->Meshes[MeshIndex];
        glBindVertexArray(Mesh.VAO);
        glDrawElements(GL_TRIANGLES, Mesh.IndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    glUseProgram(0);
}
