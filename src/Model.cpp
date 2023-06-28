#include "Model.h"

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <queue>
#include <stack>
#include <unordered_map>

#include "Util.h"

glm::mat4 assimpMatToGlmMat(aiMatrix4x4 assimpMat);
u32 prepareMeshVAO(f32 *vertexData, u32 vertexCount, u32 *indices, u32 indexCount);
u32 prepareSkinnedMeshVAO(f32 *vertexData, u32 vertexCount, u32 *indices, u32 indexCount);

SkinnedModel debugModelGLTF(const char *path)
{
    std::cout << "Loading model at: " << path << '\n';

    SkinnedModel model { };

    // Load assimp scene
    // -----------------
    const aiScene *assimpScene = aiImportFile(path,
                                              aiProcess_CalcTangentSpace |
                                              aiProcess_Triangulate |
                                              aiProcess_JoinIdenticalVertices |
                                              aiProcess_FlipUVs);

    if (!assimpScene || assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !assimpScene->mRootNode)
    {
        std::cerr << "ERROR::LOAD_MODEL::ASSIMP_READ_ERROR: " << " path: " << path << '\n' << aiGetErrorString() << '\n';
        return model;
    }

    // Parse bone data
    // ---------------
    // 1. Find armature node
    std::cout << "Looking for armature node... ";
    aiNode *armatureNode = 0;
    bool hasArmature = false;
    std::queue<aiNode *> nodeQueue;
    nodeQueue.push(assimpScene->mRootNode);
    while (!nodeQueue.empty() && !hasArmature)
    {
        aiNode *frontNode = nodeQueue.front();
        nodeQueue.pop();

        if (strcmp(frontNode->mName.C_Str(), "Armature") == 0)
        {
            armatureNode = frontNode;
            hasArmature = true;
        }

        for (i32 i = 0; i < (i32) frontNode->mNumChildren; ++i)
        {
            nodeQueue.push(frontNode->mChildren[i]);
        }
    }

    // 2. Get bone tree and transform to parent data
    if (hasArmature)
    {
        i32 bonesParsed = 0;
        std::vector<aiNode *> nodes { }; // Temp aiNode storage in the same order as bones 
        std::stack<i32> tempNodeStack { }; // Temp tree-DFS-stack that stores bone IDs for 'bones' and 'nodes' vectors
        bool isFirstIteration = true;
        while (isFirstIteration || !tempNodeStack.empty())
        {
            if (isFirstIteration)
            {

                for (i32 i = 0; i < (i32) armatureNode->mNumChildren; ++i)
                {
                    aiNode *rootBoneNode = armatureNode->mChildren[i];
                    nodes.push_back(rootBoneNode);

                    Bone rootBone { };
                    rootBone.name = rootBoneNode->mName.C_Str();
                    rootBone.transformToParent = assimpMatToGlmMat(rootBoneNode->mTransformation);
                    model.bones.push_back(rootBone);

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
                Bone *parentBone = &model.bones[parentIndex];

                for (i32 i = 0; i < (i32) parentNode->mNumChildren; ++i)
                {
                    aiNode *childNode = parentNode->mChildren[i];
                    nodes.push_back(childNode);

                    Bone childBone { };
                    childBone.name = childNode->mName.C_Str();
                    memcpy(childBone.pathToRoot, parentBone->pathToRoot, parentBone->pathNodes * sizeof(i32));
                    childBone.pathNodes = parentBone->pathNodes;
                    childBone.pathToRoot[childBone.pathNodes] = parentIndex;
                    ++childBone.pathNodes;
                    childBone.transformToParent = assimpMatToGlmMat(childNode->mTransformation);
                    model.bones.push_back(childBone);
                    // reassign because push back could've invalidated that pointer
                    parentBone = &model.bones[parentIndex];

                    tempNodeStack.push(bonesParsed);
                    ++bonesParsed;
                }
            }
        }

        std::cout << "Armature found. " << model.bones.size() << " bones:\n";

        for (i32 i = 0; i < model.bones.size(); ++i)
        {
            std::cout << "Bone #" << i << "/" << model.bones.size() << ": " << model.bones[i].name << ". Path to root: ";

            for (i32 pathIndex = 0; pathIndex < model.bones[i].pathNodes; ++pathIndex)
            {
                std::cout << model.bones[i].pathToRoot[pathIndex];

                if (pathIndex != model.bones[i].pathNodes - 1)
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

    for (i32 meshIndex = 0; meshIndex < (i32) assimpScene->mNumMeshes; ++meshIndex)
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
            for (i32 i = 0; i < model.bones.size(); ++i)
            {
                if (strcmp(model.bones[i].name.c_str(), assimpBone->mName.C_Str()) == 0)
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

            model.bones[boneID - 1].inverseBindTransform = assimpMatToGlmMat(assimpBone->mOffsetMatrix);
        }

        // Position + 4 bone ids + 4 bone weights
        f32 *meshVertexData = (f32 *)calloc(1, 11 * vertexCount * sizeof(f32));

        //std::cout << "\n  Vertices:\n";
        for (i32 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
        {
            aiVector3D vertex = assimpMesh->mVertices[vertexIndex];
            //std::cout << "  Vertex #" << vertexIndex << "/" << vertexCount << " " << vertex.x << ", " << vertex.y << ", " << vertex.z << '\n';

            f32 *meshVertexDataCursor = meshVertexData + (vertexIndex * 11);

            meshVertexDataCursor[0]  = vertex.x;
            meshVertexDataCursor[1]  = vertex.y;
            meshVertexDataCursor[2]  = vertex.z;

            meshVertexDataCursor[3]  = *((f32 *)&vertexBones[vertexIndex].boneID[0]);
            meshVertexDataCursor[4]  = *((f32 *)&vertexBones[vertexIndex].boneID[1]);
            meshVertexDataCursor[5]  = *((f32 *)&vertexBones[vertexIndex].boneID[2]);
            meshVertexDataCursor[6]  = *((f32 *)&vertexBones[vertexIndex].boneID[3]);

            meshVertexDataCursor[7]  = vertexBones[vertexIndex].boneWeight[0];
            meshVertexDataCursor[8]  = vertexBones[vertexIndex].boneWeight[1];
            meshVertexDataCursor[9]  = vertexBones[vertexIndex].boneWeight[2];
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

        model.meshes.push_back(mesh);
    }

    if (assimpScene->mNumAnimations > 0)
    {
        aiAnimation *animation = assimpScene->mAnimations[0];
        i32 channelCount = (i32) animation->mNumChannels;
        std::cout << "\nFound animation: " << animation->mName.C_Str() << ". " << channelCount << " channels" << '\n';

        model.animation.ticksDuration = (f32) animation->mDuration;
        model.animation.ticksPerSecond = (f32) animation->mTicksPerSecond;

        for (i32 channelIndex = 0; channelIndex < channelCount; ++channelIndex)
        {
            aiNodeAnim *channel = animation->mChannels[channelIndex];

            Bone *bone = 0;
            i32 boneID = 0;

            for (i32 boneIndex = 0; boneIndex < model.bones.size(); ++boneIndex)
            {
                if (strcmp(model.bones[boneIndex].name.c_str(), channel->mNodeName.C_Str()) == 0)
                {
                    bone = &model.bones[boneIndex];
                    boneID = boneIndex + 1;
                }
            }

            if (bone)
            {
                i32 positionKeyCount = channel->mNumPositionKeys;
                i32 scalingKeyCount = channel->mNumScalingKeys;
                i32 rotationKeyCount = channel->mNumRotationKeys;

                std::cout << "  Channel #" << channelIndex << "/" << channelCount 
                    << " for bone " << bone->name << "(" << boneID << "). " 
                    << "Position keys: " << positionKeyCount 
                    << "; scaling keys: " <<  scalingKeyCount
                    << "; rotation keys: " << rotationKeyCount << '\n';

                bone->positionKeys.resize(positionKeyCount);
                for (i32 i = 0; i < positionKeyCount; ++i)
                {
                    aiVectorKey *assimpPos = &channel->mPositionKeys[i];
                    PositionKey positionKey { };
                    positionKey.time = (f32) assimpPos->mTime;
                    positionKey.position = glm::vec3(assimpPos->mValue.x, assimpPos->mValue.y, assimpPos->mValue.z);
                    bone->positionKeys[i] = positionKey;
                }

                bone->scalingKeys.resize(scalingKeyCount);
                for (i32 i = 0; i < scalingKeyCount; ++i)
                {
                    aiVectorKey *assimpScale = &channel->mScalingKeys[i];
                    ScalingKey scalingKey { };
                    scalingKey.time = (f32) assimpScale->mTime;
                    scalingKey.scale = glm::vec3(assimpScale->mValue.x, assimpScale->mValue.y, assimpScale->mValue.z);
                    bone->scalingKeys[i] = scalingKey;
                }

                bone->rotationKeys.resize(rotationKeyCount);
                for (i32 i = 0; i < rotationKeyCount; ++i)
                {
                    aiQuatKey *assimpRotation = &channel->mRotationKeys[i];
                    RotationKey rotationKey { };
                    rotationKey.time = (f32) assimpRotation->mTime;
                    rotationKey.rotation = glm::quat(assimpRotation->mValue.w, assimpRotation->mValue.x, assimpRotation->mValue.y, assimpRotation->mValue.z);
                    bone->rotationKeys[i] = rotationKey;
                }
            }
        }
    }

    aiReleaseImport(assimpScene);

    return model;
}

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

    std::unordered_map<std::string, u32> loadedTextures { };

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
            meshVertexDataCursor[0]  = assimpMesh->mVertices[assimpVertexIndex].x;
            meshVertexDataCursor[1]  = assimpMesh->mVertices[assimpVertexIndex].y;
            meshVertexDataCursor[2]  = assimpMesh->mVertices[assimpVertexIndex].z;
            // Normals               
            meshVertexDataCursor[3]  = assimpMesh->mNormals[assimpVertexIndex].x;
            meshVertexDataCursor[4]  = assimpMesh->mNormals[assimpVertexIndex].y;
            meshVertexDataCursor[5]  = assimpMesh->mNormals[assimpVertexIndex].z;
            // UVs                   
            meshVertexDataCursor[6]  = assimpMesh->mTextureCoords[0][assimpVertexIndex].x;
            meshVertexDataCursor[7]  = assimpMesh->mTextureCoords[0][assimpVertexIndex].y;
            // Tangent               
            meshVertexDataCursor[8]  = assimpMesh->mTangents[assimpVertexIndex].x;
            meshVertexDataCursor[9]  = assimpMesh->mTangents[assimpVertexIndex].y;
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

glm::mat4 assimpMatToGlmMat(aiMatrix4x4 assimpMat)
{
    glm::mat4 result { };

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

void render(SkinnedModel *model, u32 shader, f32 deltaTime)
{
    glUseProgram(shader);

    if (model->animation.isRunning)
    {
        if (!model->animation.isPaused)
        {
            model->animation.currentTicks += deltaTime * model->animation.ticksPerSecond;
            if (model->animation.currentTicks >= model->animation.ticksDuration)
            {
                if (model->animation.isLooped)
                {
                    model->animation.currentTicks -= model->animation.ticksDuration;
                }
                else
                {
                    model->animation.currentTicks = 0.0f;
                    model->animation.isRunning = false;
                }
            }
        }

        std::cout << model->animation.currentTicks << '\n';

        for (i32 atlbetaBoneIndex = 0; atlbetaBoneIndex < (i32) model->bones.size(); ++atlbetaBoneIndex)
        {
            Bone bone = model->bones[atlbetaBoneIndex];

            glm::mat4 boneTransform = bone.inverseBindTransform;

            //boneTransform = bone.transformToParent * boneTransform;

            glm::mat4 scalingTransform(1.0f);
            glm::mat4 rotationTransform(1.0f);
            glm::mat4 translationTransform(1.0f);

            for (i32 i = 0; i < (i32) bone.positionKeys.size() - 1; ++i)
            {
                if (model->animation.currentTicks >= bone.positionKeys[i].time &&
                    model->animation.currentTicks < bone.positionKeys[i + 1].time)
                {
                    f32 timeBetweenFrames = bone.positionKeys[i + 1].time - bone.positionKeys[i].time;
                    f32 timeAfterPreviousFrame = model->animation.currentTicks - bone.positionKeys[i].time;
                    f32 percentNextFrame = timeAfterPreviousFrame / timeBetweenFrames;

                    glm::vec3 interpolatedPosition = 
                        (bone.positionKeys[i].position * (1 - percentNextFrame) +
                         bone.positionKeys[i + 1].position * percentNextFrame);

                    translationTransform = glm::translate(glm::mat4(1.0f), interpolatedPosition);
                    break;
                }
            }
            for (i32 i = 0; i < (i32) bone.rotationKeys.size() - 1; ++i)
            {
                if (model->animation.currentTicks >= bone.rotationKeys[i].time &&
                    model->animation.currentTicks < bone.rotationKeys[i + 1].time)
                {
                    f32 timeBetweenFrames = bone.rotationKeys[i + 1].time - bone.rotationKeys[i].time;
                    f32 timeAfterPreviousFrame = model->animation.currentTicks - bone.rotationKeys[i].time;
                    f32 percentNextFrame = timeAfterPreviousFrame / timeBetweenFrames;

                    glm::quat interpolatedRotation = glm::slerp(bone.rotationKeys[i].rotation,
                                                                bone.rotationKeys[i + 1].rotation,
                                                                percentNextFrame);

                    rotationTransform = glm::mat4_cast(interpolatedRotation);
                    break;
                }
            }
            for (i32 i = 0; i < (i32) bone.scalingKeys.size() - 1; ++i)
            {
                if (model->animation.currentTicks >= bone.scalingKeys[i].time &&
                    model->animation.currentTicks < bone.scalingKeys[i + 1].time)
                {
                    f32 timeBetweenFrames = bone.scalingKeys[i + 1].time - bone.scalingKeys[i].time;
                    f32 timeAfterPreviousFrame = model->animation.currentTicks - bone.scalingKeys[i].time;
                    f32 percentNextFrame = timeAfterPreviousFrame / timeBetweenFrames;

                    glm::vec3 scale = 
                        (bone.scalingKeys[i].scale * (1 - percentNextFrame) +
                         bone.scalingKeys[i + 1].scale * percentNextFrame);

                    scalingTransform = glm::scale(glm::mat4(1.0f), scale);
                    break;
                }
            }

            boneTransform = translationTransform * rotationTransform * scalingTransform * boneTransform;

            for (i32 parentIndex = bone.pathNodes - 1; parentIndex >= 0; --parentIndex)
            {
                Bone parentBone = model->bones[bone.pathToRoot[parentIndex]];
                //boneTransform = parentBone.transformToParent * boneTransform;

                glm::mat4 parentScalingTransform(1.0f);
                glm::mat4 parentRotationTransform(1.0f);
                glm::mat4 parentTranslationTransform(1.0f);

                for (i32 i = 0; i < (i32) parentBone.positionKeys.size() - 1; ++i)
                {
                    if (model->animation.currentTicks >= parentBone.positionKeys[i].time &&
                        model->animation.currentTicks < parentBone.positionKeys[i + 1].time)
                    {
                        f32 timeBetweenFrames = parentBone.positionKeys[i + 1].time - parentBone.positionKeys[i].time;
                        f32 timeAfterPreviousFrame = model->animation.currentTicks - parentBone.positionKeys[i].time;
                        f32 percentNextFrame = timeAfterPreviousFrame / timeBetweenFrames;

                        glm::vec3 interpolatedPosition = 
                            (parentBone.positionKeys[i].position * (1 - percentNextFrame) +
                             parentBone.positionKeys[i + 1].position * percentNextFrame);

                        parentTranslationTransform = glm::translate(glm::mat4(1.0f), interpolatedPosition);
                        break;
                    }
                }
                for (i32 i = 0; i < (i32) parentBone.rotationKeys.size() - 1; ++i)
                {
                    if (model->animation.currentTicks >= parentBone.rotationKeys[i].time &&
                        model->animation.currentTicks < parentBone.rotationKeys[i + 1].time)
                    {
                        f32 timeBetweenFrames = parentBone.rotationKeys[i + 1].time - parentBone.rotationKeys[i].time;
                        f32 timeAfterPreviousFrame = model->animation.currentTicks - parentBone.rotationKeys[i].time;
                        f32 percentNextFrame = timeAfterPreviousFrame / timeBetweenFrames;

                        glm::quat interpolatedRotation = glm::slerp(parentBone.rotationKeys[i].rotation,
                                                                    parentBone.rotationKeys[i + 1].rotation,
                                                                    percentNextFrame);

                        parentRotationTransform = glm::mat4_cast(interpolatedRotation);
                        break;
                    }
                }
                for (i32 i = 0; i < (i32) parentBone.scalingKeys.size() - 1; ++i)
                {
                    if (model->animation.currentTicks >= parentBone.scalingKeys[i].time &&
                        model->animation.currentTicks < parentBone.scalingKeys[i + 1].time)
                    {
                        f32 timeBetweenFrames = parentBone.scalingKeys[i + 1].time - parentBone.scalingKeys[i].time;
                        f32 timeAfterPreviousFrame = model->animation.currentTicks - parentBone.scalingKeys[i].time;
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

            glUniformMatrix4fv(glGetUniformLocation(shader, location.c_str()), 1, GL_FALSE, glm::value_ptr(boneTransform));
        }
    }
    else
    {
        for (i32 atlbetaBoneIndex = 0; atlbetaBoneIndex < (i32)model->bones.size(); ++atlbetaBoneIndex)
        {
            std::string location = "boneTransforms[" + std::to_string(atlbetaBoneIndex) + "]";
            glm::mat4 boneTransform(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shader, location.c_str()), 1, GL_FALSE, glm::value_ptr(boneTransform));
        }
    }

    for (i32 atlbetaMeshIndex = 0; atlbetaMeshIndex < (i32) model->meshes.size(); ++atlbetaMeshIndex)
    {
        Mesh mesh = model->meshes[atlbetaMeshIndex];
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    glUseProgram(0);
}
