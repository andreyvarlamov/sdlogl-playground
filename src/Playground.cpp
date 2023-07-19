#include <glad/glad.h>
#include <sdl2/SDL.h>
#include <sdl2/SDL_ttf.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdlib>
#include <cstdio>

#include "Common.h"
#include "Model.h"
#include "Shader.h"
#include "Text.h"
#include "Util.h"

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

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
bool AdamMovementStateButtonPressed = false;

glm::vec3 AdamPosition(-1.0f, 0.0f, -4.0f);
f32 AdamYaw = 0.0f;
i32 AdamMovementState = 0;

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
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

                int WindowWidth;
                int WindowHeight;
                SDL_GetWindowSize(Window, &WindowWidth, &WindowHeight);
                glViewport(0, 0, WindowWidth, WindowHeight);

                // Load fonts
                // ----------
                Assert(TTF_Init() != -1);
                glm::mat4 TextScale;

                u32 RenderedTextTexture =
                    DEBUG_RenderTextIntoTexture("resources/fonts/ContrailOne-Regular.ttf",
                                                "test text",
                                                &TextScale);

                // Load shaders
                // ------------
                u32 StaticMeshShader =
                    BuildShaderProgram("resources/shaders/StaticMesh.vs",
                                       "resources/shaders/BasicMesh.fs");
                u32 SkinnedMeshShader =
                    BuildShaderProgram("resources/shaders/SkinnedMesh.vs",
                                       "resources/shaders/BasicMesh.fs");

                u32 BasicTextShader =
                    BuildShaderProgram("resources/shaders/BasicText.vs",
                                       "resources/shaders/BasicText.fs");
                f32 BasicTextVertices[] = {
                    -1.0,  1.0,  0.0,  1.0,
                     1.0,  1.0,  1.0,  1.0,
                    -1.0, -1.0,  0.0,  0.0,
                     1.0, -1.0,  1.0,  0.0
                };
                u32 BasicTextIndices[] = {
                    1, 0, 2,
                    1, 2, 3
                };
                u32 BasicTextVAO;
                glGenVertexArrays(1, &BasicTextVAO);
                glBindVertexArray(BasicTextVAO);
                u32 BasicTextVBO;
                glGenBuffers(1, &BasicTextVBO);
                glBindBuffer(GL_ARRAY_BUFFER, BasicTextVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(BasicTextVertices), BasicTextVertices, GL_STATIC_DRAW);
                u32 BasicTextEBO;
                glGenBuffers(1, &BasicTextEBO);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BasicTextEBO);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(BasicTextIndices), BasicTextIndices, GL_STATIC_DRAW);

                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *) 0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *) (2 * sizeof(f32)));

                glBindVertexArray(0);

                SetUniformInt(BasicTextShader, "Text", true, 0);

                SetUniformMat4F(BasicTextShader, "TextScale", false, glm::value_ptr(TextScale));

                // Load models
                // -----------
                //skinned_model AtlbetaModel = LoadSkinnedModel("resources/models/animtest/atlbeta12.gltf");
                //AtlbetaModel.AnimationState.CurrentAnimationA = 0;
                //AtlbetaModel.AnimationState.CurrentAnimationB = 1;
                //AtlbetaModel.AnimationState.BlendingFactor = 0.0f;
                model SnowmanModel = LoadModel("resources/models/snowman/snowman.objm", true);
                model ContainerModel = LoadModel("resources/models/container/container.objm", true);
                model FloorModel = LoadModel("resources/models/primitives/floor.gltf", true);
                FloorModel.Meshes[0].DiffuseMapID = LoadTexture("resources/textures/grass.jpg", true);
                model WallModel = LoadModel("resources/models/primitives/quad.gltf", true);
                WallModel.Meshes[0].DiffuseMapID = LoadTexture("resources/textures/brickwall.jpg", true);
                WallModel.Meshes[0].NormalMapID = LoadTexture("resources/textures/brickwall_normal.jpg", true);
                skinned_model AdamModel = LoadSkinnedModel("resources/models/adam/adam.gltf", false);
                AdamModel.AnimationState.CurrentAnimationA = 0;
                AdamModel.AnimationState.CurrentAnimationB = 2;
                AdamModel.AnimationState.BlendingFactor = 0.0f;

                // Shader global uniforms
                // ----------------------
                SetUniformInt(StaticMeshShader, "DiffuseMap", true, 0);
                SetUniformInt(StaticMeshShader, "SpecularMap", false, 1);
                SetUniformInt(StaticMeshShader, "EmissionMap", false, 2);
                SetUniformInt(StaticMeshShader, "NormalMap", false, 3);

                SetUniformInt(SkinnedMeshShader, "DiffuseMap", true, 0);
                SetUniformInt(SkinnedMeshShader, "SpecularMap", false, 1);
                SetUniformInt(SkinnedMeshShader, "EmissionMap", false, 2);
                SetUniformInt(SkinnedMeshShader, "NormalMap", false, 3);

                // Light configuration
                // -------------------
                glm::vec3 LightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -0.33f));
                SetUniformVec3F(StaticMeshShader, "LightDirection", true, &LightDir[0]);
                SetUniformVec3F(SkinnedMeshShader, "LightDirection", true, &LightDir[0]);

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
                        AdamYaw += DeltaTime * 180.0f;
                        if (AdamYaw > 360.0f)
                        {
                            AdamYaw -= 360.0f;
                        }
                    }
                    if (CurrentKeyStates[SDL_SCANCODE_U])
                    {
                        AdamYaw -= DeltaTime * 180.0f;
                        if (AdamYaw < 0.0f)
                        {
                            AdamYaw += 360.0f;
                        }
                    }
                    if (CurrentKeyStates[SDL_SCANCODE_T] && !AdamMovementStateButtonPressed)
                    {
                        AdamMovementState++;
                        if (AdamMovementState > 2)
                        {
                            AdamMovementState = 0;
                        }
                        if (AdamMovementState == 0)
                        {
                            AdamModel.AnimationState.CurrentAnimationA = 0;
                        }
                        if (AdamMovementState == 1)
                        {
                            AdamModel.AnimationState.CurrentAnimationA = 3;
                        }
                        if (AdamMovementState == 2)
                        {
                            AdamModel.AnimationState.CurrentAnimationA = 2;
                        }
                        AdamMovementStateButtonPressed = true;
                    }
                    else if (!CurrentKeyStates[SDL_SCANCODE_T])
                    {
                        AdamMovementStateButtonPressed = false;
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

                    // Common transform matrices
                    glm::mat4 ProjectionTransform = glm::perspective(glm::radians(CameraFov / 2.0f), (f32)SCREEN_WIDTH / (f32)SCREEN_HEIGHT, 0.1f, 1000.0f);
                    glm::mat4 ViewTransform = glm::lookAt(CameraPosition, CameraPosition + CameraFront, CameraUp);

                    SetUniformMat4F(StaticMeshShader, "Projection", true, glm::value_ptr(ProjectionTransform));
                    SetUniformMat4F(StaticMeshShader, "View", false, glm::value_ptr(ViewTransform));
                    SetUniformVec3F(StaticMeshShader, "ViewPosition", false, &CameraPosition[0]);

                    SetUniformMat4F(SkinnedMeshShader, "Projection", true, glm::value_ptr(ProjectionTransform));
                    SetUniformMat4F(SkinnedMeshShader, "View", false, glm::value_ptr(ViewTransform));
                    SetUniformVec3F(SkinnedMeshShader, "ViewPosition", false, &CameraPosition[0]);

                    // Render models
                    // -------------

                    // floor
                    glm::mat4 ModelTransform = glm::mat4(1.0f);
                    SetUniformMat4F(StaticMeshShader, "Model", true, glm::value_ptr(ModelTransform));
                    //glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(ModelTransform)));
                    RenderModel(&FloorModel, StaticMeshShader);
                    // container 1
                    ModelTransform = glm::mat4(1.0f);
                    ModelTransform = glm::rotate(ModelTransform, CurrentFrame / 1000.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    ModelTransform = glm::scale(ModelTransform, glm::vec3(1.0f));
                    SetUniformMat4F(StaticMeshShader, "Model", false, glm::value_ptr(ModelTransform));
                    RenderModel(&ContainerModel, StaticMeshShader);
                    // container 2
                    ModelTransform = glm::mat4(1.0f);
                    ModelTransform = glm::translate(ModelTransform, glm::vec3(-1.5f, 2.0f, -2.0f));
                    ModelTransform = glm::scale(ModelTransform, glm::vec3(0.70f));
                    SetUniformMat4F(StaticMeshShader, "Model", false, glm::value_ptr(ModelTransform));
                    RenderModel(&ContainerModel, StaticMeshShader);
                    // quad wall
                    ModelTransform = glm::mat4(1.0f);
                    ModelTransform = glm::translate(ModelTransform, glm::vec3(-10.0f, 0.0f, 0.0f));
                    ModelTransform = glm::rotate(ModelTransform, CurrentFrame / 1000.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    SetUniformMat4F(StaticMeshShader, "Model", false, glm::value_ptr(ModelTransform));
                    RenderModel(&WallModel, StaticMeshShader);
                    // other side of wall (no z-fighting because faces are culled)
                    ModelTransform = glm::rotate(ModelTransform, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    SetUniformMat4F(StaticMeshShader, "Model", false, glm::value_ptr(ModelTransform));
                    RenderModel(&WallModel, StaticMeshShader);
                    // snowman
                    ModelTransform = glm::mat4(1.0f);
                    ModelTransform = glm::translate(ModelTransform, glm::vec3(0.0f, 0.0f, -5.0f));
                    ModelTransform = glm::rotate(ModelTransform, CurrentFrame / 500.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    SetUniformMat4F(StaticMeshShader, "Model", false, glm::value_ptr(ModelTransform));
                    RenderModel(&SnowmanModel, StaticMeshShader);
                    // animtest
                    //ModelTransform = glm::mat4(1.0f);
                    //ModelTransform = glm::translate(ModelTransform, glm::vec3(5.0f, 0.0f, 3.0f));
                    //ModelTransform = glm::rotate(ModelTransform, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    //ModelTransform = glm::scale(ModelTransform, glm::vec3(0.5f));
                    //SetUniformMat4F(SkinnedMeshShader, "Model", true, glm::value_ptr(ModelTransform));
                    //RenderSkinnedModel(&AtlbetaModel, SkinnedMeshShader, DeltaTime);
                    // adam
                    ModelTransform = glm::mat4(1.0f);
                    glm::vec3 AdamPositionDelta(0.0f);
                    if (AdamMovementState == 1 || AdamMovementState == 2)
                    {
                        f32 Velocity;
                        if (AdamMovementState == 1)
                        {
                            Velocity = 1.0f;
                        }
                        else
                        {
                            Velocity = 4.0f;
                        }

                        glm::vec3 Direction(sin(glm::radians(AdamYaw)), 0.0f, cos(glm::radians(AdamYaw)));

                        AdamPositionDelta = Velocity * DeltaTime * Direction;
                    }
                    AdamPosition += AdamPositionDelta;
                    ModelTransform = glm::translate(ModelTransform, AdamPosition);
                    ModelTransform = glm::rotate(ModelTransform, glm::radians(AdamYaw), glm::vec3(0.0f, 1.0f, 0.0f));
                    //ModelTransform = glm::scale(ModelTransform, glm::vec3(0.5f));
                    SetUniformMat4F(SkinnedMeshShader, "Model", true, glm::value_ptr(ModelTransform));
                    RenderSkinnedModel(&AdamModel, SkinnedMeshShader, DeltaTime);

                    // Render UI
                    // ---------

                    UseShader(BasicTextShader);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, RenderedTextTexture);
                    glBindVertexArray(BasicTextVAO);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    glBindVertexArray(0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    UseShader(0);

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
