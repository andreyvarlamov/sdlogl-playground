#include <glad/glad.h>
#include <sdl2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdio>

#include "Common.h"
#include "Model.h"
#include "Util.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

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

int
main(int Argc, char *Argv[])
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
        SDL_Window *Window = SDL_CreateWindow("Playground", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
        if (Window)
        {
            // GL Context
            // ----------
            SDL_GLContext MainContext = SDL_GL_CreateContext(Window);
            if (MainContext)
            {
                // SDL: Set relative mouse
                // -----------------------
                if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0)
                {
                    fprintf(stderr, "SDL Could not set mouse relative mode: %s\n", SDL_GetError());
                }

                // GLAD: GL function pointers
                // --------------------------
                gladLoadGLLoader(SDL_GL_GetProcAddress);
                printf("OpenGL loaded\n");
                printf("Vendor: %s\n", glGetString(GL_VENDOR));
                printf("Renderer: %s\n", glGetString(GL_RENDERER));
                printf("Version: %s\n", glGetString(GL_VERSION));

                // GL Global Settings
                glEnable(GL_DEPTH_TEST);
                glEnable(GL_CULL_FACE);

                int WindowWidth;
                int WindowHeight;
                SDL_GetWindowSize(Window, &WindowWidth, &WindowHeight);
                glViewport(0, 0, WindowWidth, WindowHeight);

                // Load shaders
                // ------------
                u32 StaticMeshShader = buildShader("resources/shaders/StaticMesh.vs", "resources/shaders/BasicMesh.fs");
                u32 SkinnedMeshShader = buildShader("resources/shaders/SkinnedMesh.vs", "resources/shaders/BasicMesh.fs");

                // Load models
                // -----------
                skinned_model AtlbetaModel = LoadSkinnedModel("resources/models/animtest/atlbeta12.gltf");
                AtlbetaModel.AnimationState.CurrentAnimationA = 0;
                AtlbetaModel.AnimationState.CurrentAnimationB = 1;
                AtlbetaModel.AnimationState.BlendingFactor = 0.0f;
                model SnowmanModel = LoadModel("resources/models/snowman/snowman.objm");
                model ContainerModel = LoadModel("resources/models/container/container.obj");
                model FloorModel = LoadModel("resources/models/primitives/floor.gltf");
                FloorModel.Meshes[0].DiffuseMapID = LoadTexture("resources/textures/grass.jpg");
                model WallModel = LoadModel("resources/models/primitives/quad.gltf");
                WallModel.Meshes[0].DiffuseMapID = LoadTexture("resources/textures/brickwall.jpg");
                WallModel.Meshes[0].NormalMapID = LoadTexture("resources/textures/brickwall_normal.jpg");

                // Shader global uniforms
                // ----------------------
                glUseProgram(StaticMeshShader);
                glUniform1i(glGetUniformLocation(StaticMeshShader, "DiffuseMap"), 0);
                glUniform1i(glGetUniformLocation(StaticMeshShader, "SpecularMap"), 1);
                glUniform1i(glGetUniformLocation(StaticMeshShader, "EmissionMap"), 2);
                glUniform1i(glGetUniformLocation(StaticMeshShader, "NormalMap"), 3);

                glUseProgram(SkinnedMeshShader);
                glUniform1i(glGetUniformLocation(SkinnedMeshShader, "DiffuseMap"), 0);
                glUniform1i(glGetUniformLocation(SkinnedMeshShader, "SpecularMap"), 1);
                glUniform1i(glGetUniformLocation(SkinnedMeshShader, "EmissionMap"), 2);
                glUniform1i(glGetUniformLocation(SkinnedMeshShader, "NormalMap"), 3);

                // Light configuration
                // -------------------
                glm::vec3 LightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -0.33f));
                glUseProgram(StaticMeshShader);
                glUniform3fv(glGetUniformLocation(StaticMeshShader, "LightDirection"), 1, &LightDir[0]);
                glUseProgram(SkinnedMeshShader);
                glUniform3fv(glGetUniformLocation(SkinnedMeshShader, "LightDirection"), 1, &LightDir[0]);

                // Game Loop
                // ---------
                f32 DeltaTime = 0.0f;
                u64 LastFrame = 0;
                SDL_Event SdlEvent;
                bool ShouldQuit = false;
                while (!ShouldQuit)
                {
                    // Poll SDL events
                    // ---------------
                    while (SDL_PollEvent(&SdlEvent))
                    {
                        if (SdlEvent.type == SDL_QUIT)
                        {
                            ShouldQuit = true;
                        }
                    }

                    // Timing
                    // ------
                    u64 CurrentFrame = SDL_GetTicks64();
                    DeltaTime = (f32)((f64)(CurrentFrame - LastFrame) / 1000.0);
                    LastFrame = CurrentFrame;

                    // Process input
                    // -------------
                    const u8 *CurrentKeyStates = SDL_GetKeyboardState(0);
                    if (CurrentKeyStates[SDL_SCANCODE_ESCAPE])
                    {
                        ShouldQuit = true;
                    }
                    if (CurrentKeyStates[SDL_SCANCODE_GRAVE] && !CameraFPSModeButtonPressed)
                    {
                        CameraFPSMode = !CameraFPSMode;
                        CameraFPSModeButtonPressed = true;
                    }
                    else if (!CurrentKeyStates[SDL_SCANCODE_GRAVE])
                    {
                        CameraFPSModeButtonPressed = false;
                    }
                    if (CurrentKeyStates[SDL_SCANCODE_Y])
                    {
                        AtlbetaModel.AnimationState.BlendingFactor -= DeltaTime;
                        if (AtlbetaModel.AnimationState.BlendingFactor < 0.0f)
                        {
                            AtlbetaModel.AnimationState.BlendingFactor = 0.0f;
                        }
                        printf("Animation factor: %f\n", AtlbetaModel.AnimationState.BlendingFactor);
                    }
                    if (CurrentKeyStates[SDL_SCANCODE_U])
                    {
                        AtlbetaModel.AnimationState.BlendingFactor += DeltaTime;
                        if (AtlbetaModel.AnimationState.BlendingFactor > 1.0f)
                        {
                            AtlbetaModel.AnimationState.BlendingFactor = 1.0f;
                        }
                        printf("Animation factor: %f\n", AtlbetaModel.AnimationState.BlendingFactor);
                    }
                    i32 MouseDeltaX, MouseDeltaY;
                    u32 MouseButtons = SDL_GetRelativeMouseState(&MouseDeltaX, &MouseDeltaY);
                    // Process camera mouse control
                    CameraYaw += MouseDeltaX * CameraMouseSensitivity;
                    CameraPitch -= MouseDeltaY * CameraMouseSensitivity;
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
                    bool IsCameraMoving = false;
                    glm::vec3 Velocity(0.0f);
                    if (CurrentKeyStates[SDL_SCANCODE_W])
                    {
                        IsCameraMoving = true;
                        Velocity += CameraFront;
                    }
                    if (CurrentKeyStates[SDL_SCANCODE_S])
                    {
                        IsCameraMoving = true;
                        Velocity -= CameraFront;
                    }
                    if (CurrentKeyStates[SDL_SCANCODE_A])
                    {
                        IsCameraMoving = true;
                        Velocity -= CameraRight;
                    }
                    if (CurrentKeyStates[SDL_SCANCODE_D])
                    {
                        IsCameraMoving = true;
                        Velocity += CameraRight;
                    }
                    if (IsCameraMoving && glm::length(Velocity) > 0.0f)
                    {
                        if (CameraFPSMode)
                        {
                            Velocity.y = 0.0f;
                        }
                        glm::vec3 NormalizedVelocity = glm::normalize(Velocity);
                        glm::vec3 PositionDelta = NormalizedVelocity * CameraMovementSpeed * DeltaTime;
                        CameraPosition += PositionDelta;
                    }

                    // Render
                    // ------
                    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    // Per-frame shader uniforms
                    // -------------------------

                    // perspective projection
                    glm::mat4 ProjectionTransform = glm::perspective(glm::radians(CameraFov / 2.0f), (f32)SCREEN_WIDTH / (f32)SCREEN_HEIGHT, 0.1f, 100.0f);
                    glUseProgram(StaticMeshShader);
                    glUniformMatrix4fv(glGetUniformLocation(StaticMeshShader, "Projection"), 1, GL_FALSE, glm::value_ptr(ProjectionTransform));
                    glUseProgram(SkinnedMeshShader);
                    glUniformMatrix4fv(glGetUniformLocation(SkinnedMeshShader, "Projection"), 1, GL_FALSE, glm::value_ptr(ProjectionTransform));
                    // camera view matrix
                    glm::mat4 ViewTransform = glm::lookAt(CameraPosition, CameraPosition + CameraFront, CameraUp);
                    glUseProgram(StaticMeshShader);
                    glUniformMatrix4fv(glGetUniformLocation(StaticMeshShader, "View"), 1, GL_FALSE, glm::value_ptr(ViewTransform));
                    glUseProgram(SkinnedMeshShader);
                    glUniformMatrix4fv(glGetUniformLocation(SkinnedMeshShader, "View"), 1, GL_FALSE, glm::value_ptr(ViewTransform));
                    // view position for light calculation
                    glUseProgram(StaticMeshShader);
                    glUniform3fv(glGetUniformLocation(StaticMeshShader, "ViewPosition"), 1, &CameraPosition[0]);
                    glUseProgram(SkinnedMeshShader);
                    glUniform3fv(glGetUniformLocation(SkinnedMeshShader, "ViewPosition"), 1, &CameraPosition[0]);
                    glUseProgram(0);

                    // Render models
                    // -------------

                    // floor
                    glm::mat4 ModelTransform = glm::mat4(1.0f);
                    glUseProgram(StaticMeshShader);
                    glUniformMatrix4fv(glGetUniformLocation(StaticMeshShader, "Model"), 1, GL_FALSE, glm::value_ptr(ModelTransform));
                    //normalMatrix = glm::transpose(glm::inverse(glm::mat3(ModelTransform)));
                    //glUniformMatrix3fv(glGetUniformLocation(StaticMeshShader, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    RenderModel(&FloorModel, StaticMeshShader);
                    // container 1
                    ModelTransform = glm::mat4(1.0f);
                    ModelTransform = glm::rotate(ModelTransform, CurrentFrame / 1000.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    ModelTransform = glm::scale(ModelTransform, glm::vec3(1.0f));
                    glUseProgram(StaticMeshShader);
                    glUniformMatrix4fv(glGetUniformLocation(StaticMeshShader, "Model"), 1, GL_FALSE, glm::value_ptr(ModelTransform));
                    RenderModel(&ContainerModel, StaticMeshShader);
                    // container 2
                    ModelTransform = glm::mat4(1.0f);
                    ModelTransform = glm::translate(ModelTransform, glm::vec3(-1.5f, 2.0f, -2.0f));
                    ModelTransform = glm::scale(ModelTransform, glm::vec3(0.70f));
                    glUseProgram(StaticMeshShader);
                    glUniformMatrix4fv(glGetUniformLocation(StaticMeshShader, "Model"), 1, GL_FALSE, glm::value_ptr(ModelTransform));
                    RenderModel(&ContainerModel, StaticMeshShader);
                    // quad wall
                    ModelTransform = glm::mat4(1.0f);
                    ModelTransform = glm::translate(ModelTransform, glm::vec3(-10.0f, 0.0f, 0.0f));
                    ModelTransform = glm::rotate(ModelTransform, CurrentFrame / 1000.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    glUseProgram(StaticMeshShader);
                    glUniformMatrix4fv(glGetUniformLocation(StaticMeshShader, "Model"), 1, GL_FALSE, glm::value_ptr(ModelTransform));
                    RenderModel(&WallModel, StaticMeshShader);
                    // other side of wall (no z-fighting because faces are culled)
                    ModelTransform = glm::rotate(ModelTransform, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    glUseProgram(StaticMeshShader);
                    glUniformMatrix4fv(glGetUniformLocation(StaticMeshShader, "Model"), 1, GL_FALSE, glm::value_ptr(ModelTransform));
                    RenderModel(&WallModel, StaticMeshShader);
                    // snowman
                    ModelTransform = glm::mat4(1.0f);
                    ModelTransform = glm::translate(ModelTransform, glm::vec3(0.0f, 0.0f, -5.0f));
                    ModelTransform = glm::rotate(ModelTransform, CurrentFrame / 500.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    glUseProgram(StaticMeshShader);
                    glUniformMatrix4fv(glGetUniformLocation(StaticMeshShader, "Model"), 1, GL_FALSE, glm::value_ptr(ModelTransform));
                    RenderModel(&SnowmanModel, StaticMeshShader);
                    // animtest
                    glUseProgram(SkinnedMeshShader);
                    ModelTransform = glm::mat4(1.0f);
                    ModelTransform = glm::translate(ModelTransform, glm::vec3(-5.0f, 0.0f, 3.0f));
                    ModelTransform = glm::rotate(ModelTransform, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    ModelTransform = glm::scale(ModelTransform, glm::vec3(0.5f));
                    glUniformMatrix4fv(glGetUniformLocation(SkinnedMeshShader, "Model"), 1, GL_FALSE, glm::value_ptr(ModelTransform));
                    RenderSkinnedModel(&AtlbetaModel, SkinnedMeshShader, DeltaTime);

                    // Swap buffer
                    // -----------
                    SDL_GL_SwapWindow(Window);
                }
            }
            else
            {
                fprintf(stderr, "Couldn't create OpenGL context: %s\n", SDL_GetError());
            }
        }
        else
        {
            fprintf(stderr, "Couldn't create SDL window:: %s\n", SDL_GetError());
        }
    }
    else
    {
        fprintf(stderr, "Couldn't initialize SDL:: %s\n", SDL_GetError());
    }

    SDL_Quit();
    return 0;
}
