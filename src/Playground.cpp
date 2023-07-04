#include <glad/glad.h>
#include <sdl2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <iostream>
#include <vector>

#include "Common.h"
#include "Model.h"
#include "Util.h"

const u32 SCREEN_WIDTH = 1280;
const u32 SCREEN_HEIGHT = 720;

glm::vec3 CameraPosition = glm::vec3(0.0f, 1.7f, 0.0f);
glm::vec3 CameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 CameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
glm::vec3 CameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
f32 CameraMovementSpeed = 3.5f;
f32 CameraMouseSensitivity = 0.05f;
f32 CameraFov = 90.0f;
f32 CameraYaw = -90.0f;
f32 CameraPitch = 0.0f;
bool CameraFPSMode = true;

bool CameraFPSModeButtonPressed = false;
bool AnimationStartButtonPressed = false;
bool AnimationLoopButtonPressed = false;
bool AnimationPauseButtonPressed = false;
bool AnimationRetriggerButtonPressed = false;
bool AnimationSwitchButtonPressed = false;

glm::vec3 calculateTangentForTriangle(const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs);
void generateTriangleVertexData(f32 *vertexData, const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs);
void generatePlaneVertexData(f32 *vertexData, const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs);
u32 prepareQuadVAO(const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs);
u32 prepareCubeVAO();

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
                u32 debugSkeletalShader = buildShader("resources/shaders/DebugSkeletal.vs", "resources/shaders/Basic.fs");

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
                u32 containerDiffuseID = LoadTexture("resources/textures/container.png");
                u32 containerSpecularID = LoadTexture("resources/textures/container_specular.png");
                u32 eyeEmissionID = LoadTexture("resources/textures/eye_emission.png");
                u32 wallDiffuseID = LoadTexture("resources/textures/brickwall.jpg");
                u32 wallNormalTexID = LoadTexture("resources/textures/brickwall_normal.jpg");
                u32 grassTextureID = LoadTexture("resources/textures/grass.jpg");
                u32 containerDiffuseID2 = LoadTexture("resources/textures/container.png");
                u32 containerSpecularID2 = LoadTexture("resources/textures/container_specular.png");
                u32 eyeEmissionID2 = LoadTexture("resources/textures/eye_emission.png");
                u32 wallDiffuseID2 = LoadTexture("resources/textures/brickwall.jpg");
                u32 wallNormalTexID2 = LoadTexture("resources/textures/brickwall_normal.jpg");
                u32 grassTextureID2 = LoadTexture("resources/textures/grass.jpg");

                // Load models
                // -----------
                skinned_model AtlbetaModel = LoadSkinnedModel("resources/models/animtest/atlbeta12.gltf");
                i32 selectedBone = 0;
                bool incBoneButtonPressed = false;
                bool decBoneButtonPressed = false;
                AtlbetaModel.AnimationState.CurrentAnimationA = 0;
                AtlbetaModel.AnimationState.CurrentAnimationB = 1;
                AtlbetaModel.AnimationState.BlendingFactor = 0.0f;
                //std::vector<Mesh> snowmanMeshes = loadModel("resources/models/snowman/snowman.objm");
                //std::vector<Mesh> containerMeshes = loadModel("resources/models/container/container.obj");

                //stbi_set_flip_vertically_on_load(true);
                //std::vector<Mesh> backpackMeshes = loadModel("resources/models/backpack/backpack.objm");
                //stbi_set_flip_vertically_on_load(false);

                // Shader global uniforms
                // ----------------------
                glUseProgram(basicShader);
                glUniform1i(glGetUniformLocation(basicShader, "DiffuseMap"), 0);
                glUseProgram(basicShader);
                glUniform1i(glGetUniformLocation(basicShader, "SpecularMap"), 1);
                glUseProgram(basicShader);
                glUniform1i(glGetUniformLocation(basicShader, "EmissionMap"), 2);
                glUseProgram(basicShader);
                glUniform1i(glGetUniformLocation(basicShader, "NormalMap"), 3);

                glUseProgram(debugSkeletalShader);
                glUniform1i(glGetUniformLocation(debugSkeletalShader, "DiffuseMap"), 0);
                glUseProgram(debugSkeletalShader);
                glUniform1i(glGetUniformLocation(debugSkeletalShader, "SpecularMap"), 1);
                glUseProgram(debugSkeletalShader);
                glUniform1i(glGetUniformLocation(debugSkeletalShader, "EmissionMap"), 2);
                glUseProgram(debugSkeletalShader);
                glUniform1i(glGetUniformLocation(debugSkeletalShader, "NormalMap"), 3);

                // Light configuration
                // -------------------
                glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -0.33f));
                glUseProgram(basicShader);
                glUniform3fv(glGetUniformLocation(basicShader, "LightDirection"), 1, &lightDir[0]);
                glUseProgram(debugSkeletalShader);
                glUniform3fv(glGetUniformLocation(debugSkeletalShader, "LightDirection"), 1, &lightDir[0]);

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
                            selectedBone = AtlbetaModel.BoneCount - 1;
                        }
                        std::cout << AtlbetaModel.Bones[selectedBone].Name << '\n';
                        decBoneButtonPressed = !decBoneButtonPressed;
                    }
                    else if (!currentKeyStates[SDL_SCANCODE_LEFT])
                    {
                        decBoneButtonPressed = false;
                    }
                    if (currentKeyStates[SDL_SCANCODE_RIGHT] && !incBoneButtonPressed)
                    {
                        ++selectedBone;
                        if (selectedBone >= AtlbetaModel.BoneCount)
                        {
                            selectedBone = 0;
                        }
                        std::cout << AtlbetaModel.Bones[selectedBone].Name << '\n';
                        incBoneButtonPressed = !incBoneButtonPressed;
                    }
                    else if (!currentKeyStates[SDL_SCANCODE_RIGHT])
                    {
                        incBoneButtonPressed = false;
                    }
                    //if (currentKeyStates[SDL_SCANCODE_T] && !AnimationSwitchButtonPressed)
                    //{
                    //    ++AtlbetaModel.AnimationState.CurrentAnimationIndex;
                    //    if (AtlbetaModel.AnimationState.CurrentAnimationIndex >= AtlbetaModel.AnimationCount)
                    //    {
                    //        AtlbetaModel.AnimationState.CurrentAnimationIndex = 0;
                    //    }
                    //    std::cout << 
                    //        AtlbetaModel.Animations[AtlbetaModel.AnimationState.CurrentAnimationIndex].Name
                    //        << '\n';
                    //    AnimationSwitchButtonPressed = !AnimationSwitchButtonPressed;
                    //}
                    //else if (!currentKeyStates[SDL_SCANCODE_T])
                    //{
                    //    AnimationSwitchButtonPressed = false;
                    //}
                    if (currentKeyStates[SDL_SCANCODE_Y])
                    {
                        AtlbetaModel.AnimationState.BlendingFactor -= deltaTime;
                        if (AtlbetaModel.AnimationState.BlendingFactor < 0.0f)
                        {
                            AtlbetaModel.AnimationState.BlendingFactor = 0.0f;
                        }
                        std::cout << "Animation factor: " << AtlbetaModel.AnimationState.BlendingFactor << '\n';
                    }
                    if (currentKeyStates[SDL_SCANCODE_U])
                    {
                        AtlbetaModel.AnimationState.BlendingFactor += deltaTime;
                        if (AtlbetaModel.AnimationState.BlendingFactor > 1.0f)
                        {
                            AtlbetaModel.AnimationState.BlendingFactor = 1.0f;
                        }
                        std::cout << "Animation factor: " << AtlbetaModel.AnimationState.BlendingFactor << '\n';
                    }
                    //if (currentKeyStates[SDL_SCANCODE_F] && !AnimationStartButtonPressed)
                    //{
                    //    AtlbetaModel.Animation.isRunning = !AtlbetaModel.Animation.isRunning;
                    //    AtlbetaModel.Animation.isPaused = false;
                    //    if (AtlbetaModel.Animation.isRunning)
                    //    {
                    //        AtlbetaModel.Animation.currentTicks = 0.0f;
                    //    }
                    //    AnimationStartButtonPressed = !AnimationStartButtonPressed;
                    //}
                    //else if (!currentKeyStates[SDL_SCANCODE_F])
                    //{
                    //    AnimationStartButtonPressed = false;
                    //}
                    //if (currentKeyStates[SDL_SCANCODE_G] && !AnimationLoopButtonPressed)
                    //{
                    //    AtlbetaModel.Animation.isLooped = !AtlbetaModel.Animation.isLooped;
                    //    AnimationLoopButtonPressed = !AnimationLoopButtonPressed;
                    //}
                    //else if (!currentKeyStates[SDL_SCANCODE_G])
                    //{
                    //    AnimationLoopButtonPressed = false;
                    //}
                    //if (currentKeyStates[SDL_SCANCODE_H] && !AnimationPauseButtonPressed)
                    //{
                    //    AtlbetaModel.Animation.isPaused = !AtlbetaModel.Animation.isPaused;
                    //    AnimationPauseButtonPressed = !AnimationPauseButtonPressed;
                    //}
                    //else if (!currentKeyStates[SDL_SCANCODE_H])
                    //{
                    //    AnimationPauseButtonPressed = false;
                    //}
                    //if (currentKeyStates[SDL_SCANCODE_J] && !AnimationRetriggerButtonPressed)
                    //{
                    //    AtlbetaModel.Animation.isRunning = true;
                    //    AtlbetaModel.Animation.isPaused = false;
                    //    AtlbetaModel.Animation.currentTicks = 0.0f;
                    //    AnimationRetriggerButtonPressed = !AnimationRetriggerButtonPressed;
                    //}
                    //else if (!currentKeyStates[SDL_SCANCODE_J])
                    //{
                    //    AnimationRetriggerButtonPressed = false;
                    //}
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
                    glUniformMatrix4fv(glGetUniformLocation(basicShader, "Projection"), 1, GL_FALSE, glm::value_ptr(projection));
                    glUseProgram(debugSkeletalShader);
                    glUniformMatrix4fv(glGetUniformLocation(debugSkeletalShader, "Projection"), 1, GL_FALSE, glm::value_ptr(projection));

                    // camera view matrix
                    glm::mat4 view = glm::lookAt(CameraPosition, CameraPosition + CameraFront, CameraUp);
                    glUseProgram(basicShader);
                    glUniformMatrix4fv(glGetUniformLocation(basicShader, "View"), 1, GL_FALSE, glm::value_ptr(view));
                    glUseProgram(debugSkeletalShader);
                    glUniformMatrix4fv(glGetUniformLocation(debugSkeletalShader, "View"), 1, GL_FALSE, glm::value_ptr(view));

                    // view position for light calculation
                    glUseProgram(basicShader);
                    glUniform3fv(glGetUniformLocation(basicShader, "ViewPosition"), 1, &CameraPosition[0]);
                    glUseProgram(debugSkeletalShader);
                    glUniform3fv(glGetUniformLocation(debugSkeletalShader, "ViewPosition"), 1, &CameraPosition[0]);

                    glUseProgram(0);

                    glm::mat4 model = glm::mat4(1.0f);
                    glm::mat3 normalMatrix = glm::mat3(1.0f);

                    glUseProgram(basicShader);

                    // floor
                    model = glm::mat4(1.0f);
                    model = glm::scale(model, glm::vec3(25.0f));
                    glUniformMatrix4fv(glGetUniformLocation(basicShader, "Model"), 1, GL_FALSE, glm::value_ptr(model));
                    //normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    //glUniformMatrix3fv(glGetUniformLocation(basicShader, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
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
                    model = glm::translate(model, glm::vec3(-10.0f, 0.0f, 0.0f));
                    model = glm::rotate(model, currentFrame / 1000.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(glGetUniformLocation(basicShader, "Model"), 1, GL_FALSE, glm::value_ptr(model));
                    glBindVertexArray(wallVAO);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallDiffuseID);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, wallNormalTexID);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                    // other side of wall (no z-fighting because faces are culled)
                    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(glGetUniformLocation(basicShader, "Model"), 1, GL_FALSE, glm::value_ptr(model));
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
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(-5.0f, 0.0f, 3.0f));
                    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(0.5f));
                    glUniformMatrix4fv(glGetUniformLocation(debugSkeletalShader, "Model"), 1, GL_FALSE, glm::value_ptr(model));
                    Render(&AtlbetaModel, debugSkeletalShader, deltaTime);

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
