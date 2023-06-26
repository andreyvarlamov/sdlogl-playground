#include <glad/glad.h>
#include <sdl2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/quaternion.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <unordered_map>

#include "Common.h"

const u32 SCREEN_WIDTH = 1280;
const u32 SCREEN_HEIGHT = 720;

glm::vec3 CameraPosition = glm::vec3(0.0f, 1.7f, 3.0f);
glm::vec3 CameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 CameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
glm::vec3 CameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
f32 CameraMovementSpeed = 3.5f;
f32 CameraMouseSensitivity = 0.05f;
f32 CameraFov = 90.0f;
f32 CameraYaw = -90.0f;
f32 CameraPitch = 0.0f;
bool CameraFPSMode = true;

bool CameraFPSModeButtonPressed = false;

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
};

struct SkinnedModel
{
    std::vector<Mesh> meshes;
    std::vector<Bone> bones;

    AnimationData animation;
};

std::string readFile(const char *path);
u32 loadTexture(const char* path);
std::vector<Mesh> loadModel(const char *path);
SkinnedModel debugModelGLTF(const char *path);
glm::vec3 calculateTangentForTriangle(const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs);
void generateTriangleVertexData(f32 *vertexData, const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs);
void generatePlaneVertexData(f32 *vertexData, const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs);
u32 prepareQuadVAO(const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs);
u32 prepareCubeVAO();
u32 prepareMeshVAO(f32 *vertexData, u32 vertexCount, u32 *indices, u32 indexCount);
u32 prepareSkinnedMeshVAO(f32 *vertexData, u32 vertexCount, u32 *indices, u32 indexCount);
u32 buildShader(const char *vertexPath, const char *fragmentPath);

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) >= 0)
    {
        // SDL/GL attributes
        // -----------------
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

        // SDL window
        // ----------
        SDL_Window *window = SDL_CreateWindow("Playground", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
        if (window)
        {
            // GL Context
            // ----------
            SDL_GLContext mainContext = SDL_GL_CreateContext(window);
            if (mainContext)
            {
                // SDL: Set relative mouse
                // -----------------------
                if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0)
                {
                    std::cerr << "SDL Could not set mouse relative mode: " << SDL_GetError() << '\n';
                }

                // GLAD: GL function pointers
                // --------------------------
                gladLoadGLLoader(SDL_GL_GetProcAddress);
                std::cout << "OpenGL loaded\n";
                std::cout << "Vendor: " << glGetString(GL_VENDOR) << '\n';
                std::cout << "Renderer: " << glGetString(GL_RENDERER) << '\n';
                std::cout << "Version: " << glGetString(GL_VERSION) << '\n';

                // GL Global Settings
                glEnable(GL_DEPTH_TEST);
                glEnable(GL_CULL_FACE);

                int windowWidth;
                int windowHeight;
                SDL_GetWindowSize(window, &windowWidth, &windowHeight);
                glViewport(0, 0, windowWidth, windowHeight);

                // Load shaders
                // ------------
                u32 basicShader = buildShader("resources/shaders/Basic.vs", "resources/shaders/Basic.fs");
                u32 debugSkeletalShader = buildShader("resources/shaders/DebugSkeletal.vs", "resources/shaders/DebugSkeletal.fs");

                // Vertex data
                // -----------
                // cube
                u32 cubeVAO = prepareCubeVAO();

                // floor plane
                glm::vec3 floorPositions[] = {
                    glm::vec3(-1.0f, 0.0f, -1.0f),
                    glm::vec3(-1.0f, 0.0f, 1.0f),
                    glm::vec3(1.0f, 0.0f, 1.0f),
                    glm::vec3(1.0f, 0.0f, -1.0f),
                };
                glm::vec3 floorNormal(0.0f, 1.0f, 0.0f);
                glm::vec2 floorUvs[] = {
                    glm::vec2(0.0f, 25.0f),
                    glm::vec2(0.0f, 0.0f),
                    glm::vec2(25.0f, 0.0f),
                    glm::vec2(25.0f, 25.0f),
                };
                u32 floorVAO = prepareQuadVAO(floorPositions, floorNormal, floorUvs);

                // wall
                glm::vec3 wallPositions[] = {
                    glm::vec3(-1.0f,  2.0f, 0.0f),
                    glm::vec3(-1.0f,  0.0f, 0.0f),
                    glm::vec3( 1.0f,  0.0f, 0.0f),
                    glm::vec3( 1.0f,  2.0f, 0.0f)
                };
                glm::vec3 wallNormal(0.0f, 0.0f, 1.0f);
                glm::vec2 wallUvs[] = {
                    glm::vec2(0.0f, 1.0f),
                    glm::vec2(0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f)
                };
                u32 wallVAO = prepareQuadVAO(wallPositions, wallNormal, wallUvs);

                // Load textures
                // -------------
                u32 containerDiffuseID = loadTexture("resources/textures/container.png");
                u32 containerSpecularID = loadTexture("resources/textures/container_specular.png");
                u32 eyeEmissionID = loadTexture("resources/textures/eye_emission.png");
                u32 wallDiffuseID = loadTexture("resources/textures/brickwall.jpg");
                u32 wallNormalTexID = loadTexture("resources/textures/brickwall_normal.jpg");
                u32 grassTextureID = loadTexture("resources/textures/grass.jpg");

                // Load models
                // -----------
                SkinnedModel atlbetaModel = debugModelGLTF("resources/models/animtest/atlbeta02.gltf");
                i32 selectedBone = 0;
                bool incBoneButtonPressed = false;
                bool decBoneButtonPressed = false;
                //std::vector<Mesh> snowmanMeshes = loadModel("resources/models/snowman/snowman.objm");
                //std::vector<Mesh> containerMeshes = loadModel("resources/models/container/container.obj");

                //stbi_set_flip_vertically_on_load(true);
                //std::vector<Mesh> backpackMeshes = loadModel("resources/models/backpack/backpack.objm");
                //stbi_set_flip_vertically_on_load(false);

                // Shader global uniforms
                // ----------------------
                glUseProgram(basicShader);
                glUniform1i(glGetUniformLocation(basicShader, "diffuseMap"), 0);
                glUseProgram(basicShader);
                glUniform1i(glGetUniformLocation(basicShader, "specularMap"), 1);
                glUseProgram(basicShader);
                glUniform1i(glGetUniformLocation(basicShader, "emissionMap"), 2);
                glUseProgram(basicShader);
                glUniform1i(glGetUniformLocation(basicShader, "normalMap"), 3);

                // Light configuration
                // -------------------
                glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -0.33f));
                glUniform3fv(glGetUniformLocation(basicShader, "lightDirection"), 1, &lightDir[0]);

                // Game Loop
                // ---------
                f32 deltaTime = 0.0f;
                u64 lastFrame = 0;
                SDL_Event event;
                bool quit = false;
                while (!quit)
                {
                    // Poll SDL events
                    // ---------------
                    while (SDL_PollEvent(&event))
                    {
                        if (event.type == SDL_QUIT)
                        {
                            quit = true;
                        }
                    }

                    // Timing
                    // ------
                    u64 currentFrame = SDL_GetTicks64();
                    deltaTime = (f32)((f64)(currentFrame - lastFrame) / 1000.0);
                    lastFrame = currentFrame;
#if 0
                    std::cout << "DT:" << deltaTime << '\n';
#endif

                    // Process input
                    // -------------
                    const u8 *currentKeyStates = SDL_GetKeyboardState(0);
                    if (currentKeyStates[SDL_SCANCODE_ESCAPE])
                    {
                        quit = true;
                    }
                    if (currentKeyStates[SDL_SCANCODE_GRAVE] && !CameraFPSModeButtonPressed)
                    {
                        CameraFPSMode = !CameraFPSMode;
                        CameraFPSModeButtonPressed = true;
                    }
                    else if (!currentKeyStates[SDL_SCANCODE_GRAVE])
                    {
                        CameraFPSModeButtonPressed = false;
                    }
                    if (currentKeyStates[SDL_SCANCODE_LEFT] && !decBoneButtonPressed)
                    {
                        --selectedBone;
                        if (selectedBone < 0)
                        {
                            selectedBone = (i32) atlbetaModel.bones.size();
                        }
                        std::cout << ((selectedBone == 0) ? "None" : atlbetaModel.bones[selectedBone - 1].name) << '\n';
                        decBoneButtonPressed = !decBoneButtonPressed;
                    }
                    else if (!currentKeyStates[SDL_SCANCODE_LEFT])
                    {
                        decBoneButtonPressed = false;
                    }
                    if (currentKeyStates[SDL_SCANCODE_RIGHT] && !incBoneButtonPressed)
                    {
                        ++selectedBone;
                        if (selectedBone > (i32) atlbetaModel.bones.size())
                        {
                            selectedBone = 0;
                        }
                        std::cout << ((selectedBone == 0) ? "None" : atlbetaModel.bones[selectedBone - 1].name) << '\n';
                        incBoneButtonPressed = !incBoneButtonPressed;
                    }
                    else if (!currentKeyStates[SDL_SCANCODE_RIGHT])
                    {
                        incBoneButtonPressed = false;
                    }
                    i32 mouseDeltaX, mouseDeltaY;
                    u32 mouseButtons = SDL_GetRelativeMouseState(&mouseDeltaX, &mouseDeltaY);
                    // Process camera mouse control
                    CameraYaw += mouseDeltaX * CameraMouseSensitivity;
                    CameraPitch -= mouseDeltaY * CameraMouseSensitivity;
                    if (CameraPitch > 89.0f)
                    {
                        CameraPitch = 89.0f;
                    }
                    else if (CameraPitch < -89.0f)
                    {
                        CameraPitch = -89.0f;
                    }
                    CameraFront.x = cos(glm::radians(CameraYaw)) * cos(glm::radians(CameraPitch));
                    CameraFront.y = sin(glm::radians(CameraPitch));
                    CameraFront.z = sin(glm::radians(CameraYaw)) * cos(glm::radians(CameraPitch));
                    CameraFront = glm::normalize(CameraFront);
                    CameraRight = glm::normalize(glm::cross(CameraFront, CameraUp));
                    // Process camera keyboard movement
                    bool moving = false;
                    glm::vec3 velocity(0.0f);
                    if (currentKeyStates[SDL_SCANCODE_W])
                    {
                        moving = true;
                        velocity += CameraFront;
                    }
                    if (currentKeyStates[SDL_SCANCODE_S])
                    {
                        moving = true;
                        velocity -= CameraFront;
                    }
                    if (currentKeyStates[SDL_SCANCODE_A])
                    {
                        moving = true;
                        velocity -= CameraRight;
                    }
                    if (currentKeyStates[SDL_SCANCODE_D])
                    {
                        moving = true;
                        velocity += CameraRight;
                    }
                    if (moving && glm::length(velocity) > 0.0f)
                    {
                        if (CameraFPSMode)
                        {
                            velocity.y = 0.0f;
                        }
                        glm::vec3 normalizedVelocity = glm::normalize(velocity);
                        glm::vec3 positionDelta = normalizedVelocity * CameraMovementSpeed * deltaTime;
                        CameraPosition += positionDelta;
#if 0
                        std::cout << "cam mov dir:" << glm::to_string(normalizedVelocity) <<
                            " | mov vel:" << glm::length(positionDelta) << '\n';
#endif
                    }
#if 0
                    std::cout << "cam front:" << glm::to_string(CameraFront) <<
                        " | position:" << glm::to_string(CameraPosition) << '\n';
#endif

                    // Render
                    // ------
                    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


                    // perspective projection
                    glm::mat4 projection = glm::perspective(glm::radians(CameraFov / 2.0f), (f32)SCREEN_WIDTH / (f32)SCREEN_HEIGHT, 0.1f, 100.0f);
                    glUseProgram(basicShader);
                    glUniformMatrix4fv(glGetUniformLocation(basicShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

                    // camera view matrix
                    glm::mat4 view = glm::lookAt(CameraPosition, CameraPosition + CameraFront, CameraUp);
                    glUseProgram(basicShader);
                    glUniformMatrix4fv(glGetUniformLocation(basicShader, "view"), 1, GL_FALSE, glm::value_ptr(view));

                    // view position for light calculation
                    glUseProgram(basicShader);
                    glUniform3fv(glGetUniformLocation(basicShader, "viewPosition"), 1, &CameraPosition[0]);

                    glUseProgram(0);

                    glm::mat4 model = glm::mat4(1.0f);
                    glm::mat3 normalMatrix = glm::mat3(1.0f);

                    glUseProgram(basicShader);

                    // floor
                    model = glm::mat4(1.0f);
                    model = glm::scale(model, glm::vec3(25.0f));
                    glUniformMatrix4fv(glGetUniformLocation(basicShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    glUniformMatrix3fv(glGetUniformLocation(basicShader, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, grassTextureID);
                    glBindVertexArray(floorVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                    glBindVertexArray(0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);

                    // container 1
                    //model = glm::mat4(1.0f);
                    //model = glm::rotate(model, currentFrame / 1000.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    //model = glm::scale(model, glm::vec3(1.0f));
                    //glUniformMatrix4fv(glGetUniformLocation(basicShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    //normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    //glUniformMatrix3fv(glGetUniformLocation(basicShader, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    //for (u32 containerMeshIndex = 0; containerMeshIndex < containerMeshes.size(); ++containerMeshIndex)
                    //{
                    //    Mesh mesh = containerMeshes[containerMeshIndex];
                    //    glActiveTexture(GL_TEXTURE0);
                    //    glBindTexture(GL_TEXTURE_2D, mesh.diffuseMapID);
                    //    glActiveTexture(GL_TEXTURE1);
                    //    glBindTexture(GL_TEXTURE_2D, mesh.specularMapID);
                    //    glActiveTexture(GL_TEXTURE2);
                    //    glBindTexture(GL_TEXTURE_2D, mesh.emissionMapID);
                    //    glActiveTexture(GL_TEXTURE3);
                    //    glBindTexture(GL_TEXTURE_2D, mesh.normalMapID);
                    //    glBindVertexArray(mesh.vao);
                    //    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
                    //    glBindVertexArray(0);
                    //    glActiveTexture(GL_TEXTURE0);
                    //    glBindTexture(GL_TEXTURE_2D, 0);
                    //    glActiveTexture(GL_TEXTURE1);
                    //    glBindTexture(GL_TEXTURE_2D, 0);
                    //    glActiveTexture(GL_TEXTURE2);
                    //    glBindTexture(GL_TEXTURE_2D, 0);
                    //    glActiveTexture(GL_TEXTURE3);
                    //    glBindTexture(GL_TEXTURE_2D, 0);
                    //}

                    // container 2
                    //model = glm::mat4(1.0f);
                    //model = glm::translate(model, glm::vec3(-1.5f, 2.0f, -2.0f));
                    //model = glm::scale(model, glm::vec3(0.70f));
                    //glUniformMatrix4fv(glGetUniformLocation(basicShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    //normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    //glUniformMatrix3fv(glGetUniformLocation(basicShader, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    //for (u32 containerMeshIndex = 0; containerMeshIndex < containerMeshes.size(); ++containerMeshIndex)
                    //{
                    //    Mesh mesh = containerMeshes[containerMeshIndex];
                    //    glActiveTexture(GL_TEXTURE0);
                    //    glBindTexture(GL_TEXTURE_2D, mesh.diffuseMapID);
                    //    glActiveTexture(GL_TEXTURE1);
                    //    glBindTexture(GL_TEXTURE_2D, mesh.specularMapID);
                    //    glActiveTexture(GL_TEXTURE2);
                    //    glBindTexture(GL_TEXTURE_2D, mesh.emissionMapID);
                    //    glActiveTexture(GL_TEXTURE3);
                    //    glBindTexture(GL_TEXTURE_2D, mesh.normalMapID);
                    //    glBindVertexArray(mesh.vao);
                    //    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
                    //    glBindVertexArray(0);
                    //    glActiveTexture(GL_TEXTURE0);
                    //    glBindTexture(GL_TEXTURE_2D, 0);
                    //    glActiveTexture(GL_TEXTURE1);
                    //    glBindTexture(GL_TEXTURE_2D, 0);
                    //    glActiveTexture(GL_TEXTURE2);
                    //    glBindTexture(GL_TEXTURE_2D, 0);
                    //    glActiveTexture(GL_TEXTURE3);
                    //    glBindTexture(GL_TEXTURE_2D, 0);
                    //}
                    
                    // quad wall
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(-5.0f, 0.0f, 0.0f));
                    model = glm::rotate(model, currentFrame / 1000.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(glGetUniformLocation(basicShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    glUniformMatrix3fv(glGetUniformLocation(basicShader, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    glBindVertexArray(wallVAO);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallDiffuseID);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, wallNormalTexID);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                    // other side of wall (no z-fighting because faces are culled)
                    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(glGetUniformLocation(basicShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    glUniformMatrix3fv(glGetUniformLocation(basicShader, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glBindVertexArray(0);

                    // snowman
                    //model = glm::mat4(1.0f);
                    //model = glm::translate(model, glm::vec3(0.0f, 0.0f, -5.0f));
                    //model = glm::rotate(model, currentFrame / 1000.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    //glUniformMatrix4fv(glGetUniformLocation(basicShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    //normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    //glUniformMatrix3fv(glGetUniformLocation(basicShader, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    //for (u32 snowmanMeshIndex = 0; snowmanMeshIndex < snowmanMeshes.size(); ++snowmanMeshIndex)
                    //{
                    //    Mesh mesh = snowmanMeshes[snowmanMeshIndex];
                    //    glActiveTexture(GL_TEXTURE0);
                    //    glBindTexture(GL_TEXTURE_2D, mesh.diffuseMapID);
                    //    glBindVertexArray(mesh.vao);
                    //    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
                    //    glBindVertexArray(0);
                    //    glActiveTexture(GL_TEXTURE0);
                    //    glBindTexture(GL_TEXTURE_2D, 0);
                    //}

                    // backpack
                    //model = glm::mat4(1.0f);
                    //model = glm::translate(model, glm::vec3(0.0f, 1.0f, 5.0f));
                    //model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    //model = glm::scale(model, glm::vec3(0.5f));
                    //glUniformMatrix4fv(glGetUniformLocation(basicShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    //normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    //glUniformMatrix3fv(glGetUniformLocation(basicShader, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    //for (u32 backpackMeshIndex = 0; backpackMeshIndex < backpackMeshes.size(); ++backpackMeshIndex)
                    //{
                    //    Mesh mesh = backpackMeshes[backpackMeshIndex];
                    //    glActiveTexture(GL_TEXTURE0);
                    //    glBindTexture(GL_TEXTURE_2D, mesh.diffuseMapID);
                    //    glBindVertexArray(mesh.vao);
                    //    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
                    //    glBindVertexArray(0);
                    //    glActiveTexture(GL_TEXTURE0);
                    //    glBindTexture(GL_TEXTURE_2D, 0);
                    //}

                    // animtest
                    glUseProgram(debugSkeletalShader);
                    glUniformMatrix4fv(glGetUniformLocation(debugSkeletalShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
                    glUniformMatrix4fv(glGetUniformLocation(debugSkeletalShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 5.0f));
                    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(1.0f));
                    glUniformMatrix4fv(glGetUniformLocation(debugSkeletalShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    glUniform1i(glGetUniformLocation(debugSkeletalShader, "selectedBone"), selectedBone);
                    for (u32 atlbetaMeshIndex = 0; atlbetaMeshIndex < atlbetaModel.meshes.size(); ++atlbetaMeshIndex)
                    {
                        Mesh mesh = atlbetaModel.meshes[atlbetaMeshIndex];
                        glBindVertexArray(mesh.vao);
                        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
                        glBindVertexArray(0);
                    }

                    glUseProgram(0);

                    // Swap buffer
                    // -----------
                    SDL_GL_SwapWindow(window);
                }
            }
            else
            {
                std::cerr << "Couldn't create OpenGL context: " << SDL_GetError() << '\n';
            }
        }
        else
        {
            std::cerr << "Couldn't create SDL window: " << SDL_GetError() << '\n';
        }
    }
    else
    {
        std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << '\n';
    }

    SDL_Quit();
    return 0;
}

std::string readFile(const char *path)
{
    std::string content;
    std::ifstream fileStream;
    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        fileStream.open(path);
        std::stringstream contentStream;
        contentStream << fileStream.rdbuf();
        fileStream.close();
        content = contentStream.str();
    }
    catch (std::ifstream::failure &e)
    {
        std::cerr << "ERROR::SHADER::FILE_NOT_READ: " << e.what() << '\n';
    }

    return content;
}

// TODO: STBI is too slow; switch back to SDL image
u32 loadTexture(const char* path)
{
    int width, height, nrComponents;
    u8 *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
        {
            format = GL_RED;
        }
        else if(nrComponents == 3)
        {
            format = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            format = GL_RGBA;
        }
        else
        {
            std::cerr << "Unknown texture format at path: " << path << '\n';
            stbi_image_free(data);
            return 0;
        }

        u32 textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(data);
        return textureID;
    }
    else
    {
        std::cerr << "Texture failed to load at path: " << path << '\n';
        return 0;
    }
}



glm::mat4 assimpMatToGlmMat(aiMatrix4x4 assimpMat)
{
    glm::mat4 result { };

    result[0][0] = assimpMat.a1; result[0][1] = assimpMat.a2; result[0][2] = assimpMat.a3; result[0][3] = assimpMat.a4;
    result[1][0] = assimpMat.b1; result[1][1] = assimpMat.b2; result[1][2] = assimpMat.b3; result[1][3] = assimpMat.b4;
    result[2][0] = assimpMat.c1; result[2][1] = assimpMat.c2; result[2][2] = assimpMat.c3; result[2][3] = assimpMat.c4;
    result[3][0] = assimpMat.d1; result[3][1] = assimpMat.d2; result[3][2] = assimpMat.d3; result[3][3] = assimpMat.d4;

    return result;
}

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

glm::vec3 calculateTangentForTriangle(const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs)
{
    glm::vec3 result;

    glm::vec3 edge1 = positions[1] - positions[0];
    glm::vec3 edge2 = positions[2] - positions[0];
    glm::vec2 deltaUV1 = uvs[1] - uvs[0];
    glm::vec2 deltaUV2 = uvs[2] - uvs[0];
    f32 f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
    result.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    result.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    result.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

    return result;
}

void generateTriangleVertexData(f32 *vertexData, const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs)
{
    glm::vec3 tangent = calculateTangentForTriangle(positions, normal, uvs);

    f32 *vertexDataCursor = vertexData;
    vertexDataCursor[0]  = positions[0].x;
    vertexDataCursor[1]  = positions[0].y;
    vertexDataCursor[2]  = positions[0].z;
    vertexDataCursor[3]  = normal.x;
    vertexDataCursor[4]  = normal.y;
    vertexDataCursor[5]  = normal.z;
    vertexDataCursor[6]  = uvs[0].x;
    vertexDataCursor[7]  = uvs[0].y;
    vertexDataCursor[8]  = tangent.x;
    vertexDataCursor[9]  = tangent.y;
    vertexDataCursor[10] = tangent.z;

    vertexDataCursor += 11;
    vertexDataCursor[0]  = positions[1].x;
    vertexDataCursor[1]  = positions[1].y;
    vertexDataCursor[2]  = positions[1].z;
    vertexDataCursor[3]  = normal.x;
    vertexDataCursor[4]  = normal.y;
    vertexDataCursor[5]  = normal.z;
    vertexDataCursor[6]  = uvs[1].x;
    vertexDataCursor[7]  = uvs[1].y;
    vertexDataCursor[8]  = tangent.x;
    vertexDataCursor[9]  = tangent.y;
    vertexDataCursor[10] = tangent.z;

    vertexDataCursor += 11;
    vertexDataCursor[0]  = positions[2].x;
    vertexDataCursor[1]  = positions[2].y;
    vertexDataCursor[2]  = positions[2].z;
    vertexDataCursor[3]  = normal.x;
    vertexDataCursor[4]  = normal.y;
    vertexDataCursor[5]  = normal.z;
    vertexDataCursor[6]  = uvs[2].x;
    vertexDataCursor[7]  = uvs[2].y;
    vertexDataCursor[8]  = tangent.x;
    vertexDataCursor[9]  = tangent.y;
    vertexDataCursor[10] = tangent.z;
}

void generatePlaneVertexData(f32 *vertexData, const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs)
{
    // Triangle 1
    f32 *vertexDataCursor = vertexData;
    glm::vec3 triangle1Positions[] = {
        positions[0], positions[1], positions[2]
    };
    glm::vec2 triangle1Uvs[] = {
        uvs[0], uvs[1], uvs[2]
    };
    generateTriangleVertexData(vertexDataCursor, triangle1Positions, normal, triangle1Uvs);

    // Triangle 2
    vertexDataCursor += 33;
    glm::vec3 triangle2Positions[] = {
        positions[0], positions[2], positions[3]
    };
    glm::vec2 triangle2Uvs[] = {
        uvs[0], uvs[2], uvs[3]
    };
    generateTriangleVertexData(vertexDataCursor, triangle2Positions, normal, triangle2Uvs);
}

u32 prepareQuadVAO(const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs)
{
    // 1 quad - 2 triangles - 6 vertices, 11 floats per vertex (position.xyz,normal.xyz,uv.xy,tangent.xyz) = 66 floats.
    f32 vertexData[66] = { 0 };

    generatePlaneVertexData(vertexData, positions, normal, uvs);

    u32 quadVBO;
    glGenBuffers(1, &quadVBO);
    u32 quadVAO;
    glGenVertexArrays(1, &quadVAO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(f32), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(f32), (void *)(3 * sizeof(f32)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(f32), (void *)(6 * sizeof(f32)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(f32), (void *)(8 * sizeof(f32)));
    glBindVertexArray(0);

    return quadVAO;
}

u32 prepareCubeVAO()
{
    // 66 floats per face; 6 faces
    f32 vertexData[396] = { 0 };
    glm::vec3 facePositions[4];
    glm::vec3 faceNormal;
    glm::vec2 faceUvs[4];
    faceUvs[0] = glm::vec2(1.0f, 1.0f);
    faceUvs[1] = glm::vec2(1.0f, 0.0f);
    faceUvs[2] = glm::vec2(0.0f, 0.0f);
    faceUvs[3] = glm::vec2(0.0f, 1.0f);

    // Back face
    f32 *vertexDataCursor = vertexData;
    facePositions[0] = glm::vec3( 0.5f, 1.0f, -0.5f);
    facePositions[1] = glm::vec3( 0.5f, 0.0f, -0.5f);
    facePositions[2] = glm::vec3(-0.5f, 0.0f, -0.5f);
    facePositions[3] = glm::vec3(-0.5f, 1.0f, -0.5f);
    faceNormal = glm::vec3(0.0f, 0.0f, -1.0f);
    generatePlaneVertexData(vertexDataCursor, facePositions, faceNormal, faceUvs);

    // Front face
    vertexDataCursor += 66;
    facePositions[0] = glm::vec3(-0.5f, 1.0f,  0.5f);
    facePositions[1] = glm::vec3(-0.5f, 0.0f,  0.5f);
    facePositions[2] = glm::vec3( 0.5f, 0.0f,  0.5f);
    facePositions[3] = glm::vec3( 0.5f, 1.0f,  0.5f);
    faceNormal = glm::vec3(0.0f, 0.0f, 1.0f);
    generatePlaneVertexData(vertexDataCursor, facePositions, faceNormal, faceUvs);

    // Left face
    vertexDataCursor += 66;
    facePositions[0] = glm::vec3(-0.5f, 1.0f, -0.5f);
    facePositions[1] = glm::vec3(-0.5f, 0.0f, -0.5f);
    facePositions[2] = glm::vec3(-0.5f, 0.0f,  0.5f);
    facePositions[3] = glm::vec3(-0.5f, 1.0f,  0.5f);
    faceNormal = glm::vec3(-1.0f, 0.0f, 0.0f);
    generatePlaneVertexData(vertexDataCursor, facePositions, faceNormal, faceUvs);

    // Right face
    vertexDataCursor += 66;
    facePositions[0] = glm::vec3( 0.5f, 1.0f,  0.5f);
    facePositions[1] = glm::vec3( 0.5f, 0.0f,  0.5f);
    facePositions[2] = glm::vec3( 0.5f, 0.0f, -0.5f);
    facePositions[3] = glm::vec3( 0.5f, 1.0f, -0.5f);
    faceNormal = glm::vec3(1.0f, 0.0f, 0.0f);
    generatePlaneVertexData(vertexDataCursor, facePositions, faceNormal, faceUvs);

    // Bottom face
    vertexDataCursor += 66;
    facePositions[0] = glm::vec3(-0.5f, 0.0f,  0.5f);
    facePositions[1] = glm::vec3(-0.5f, 0.0f, -0.5f);
    facePositions[2] = glm::vec3( 0.5f, 0.0f, -0.5f);
    facePositions[3] = glm::vec3( 0.5f, 0.0f,  0.5f);
    faceNormal = glm::vec3(0.0f, -1.0f, 0.0f);
    generatePlaneVertexData(vertexDataCursor, facePositions, faceNormal, faceUvs);

    // Top face
    vertexDataCursor += 66;
    facePositions[0] = glm::vec3(-0.5f, 1.0f, -0.5f);
    facePositions[1] = glm::vec3(-0.5f, 1.0f,  0.5f);
    facePositions[2] = glm::vec3( 0.5f, 1.0f,  0.5f);
    facePositions[3] = glm::vec3( 0.5f, 1.0f, -0.5f);
    faceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    generatePlaneVertexData(vertexDataCursor, facePositions, faceNormal, faceUvs);

    u32 quadVBO;
    glGenBuffers(1, &quadVBO);
    u32 quadVAO;
    glGenVertexArrays(1, &quadVAO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(f32), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(f32), (void *)(3 * sizeof(f32)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(f32), (void *)(6 * sizeof(f32)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(f32), (void *)(8 * sizeof(f32)));
    glBindVertexArray(0);

    return quadVAO;
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

u32 buildShader(const char *vertexPath, const char *fragmentPath)
{
    u32 vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    std::string vertexShaderSource = readFile(vertexPath);
    const char *vertexShaderSourceCStr = vertexShaderSource.c_str();
    glShaderSource(vertexShader, 1, &vertexShaderSourceCStr, 0);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, 0, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << '\n';
    }
    u32 fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    std::string fragmentShaderSource = readFile(fragmentPath);
    const char *fragmentShaderSourceCStr = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderSourceCStr, 0);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, 0, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << '\n';
    }
    u32 shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, 0, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << '\n';
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}
