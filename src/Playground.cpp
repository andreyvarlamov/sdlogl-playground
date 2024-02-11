#include <glad/glad.h>
#include <sdl2/SDL.h>
#include <sdl2/SDL_ttf.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdlib>
#include <cstdio>

#include "Collisions.h"
#include "Common.h"
#include "DebugUI.h"
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

bool DebugUIToggle = true;

bool CameraFPSModeButtonPressed = false;
bool AdamMovementStateButtonPressed = false;
bool DebugUIToggleButtonPressed = false;
bool DebugCollisionsForceResolvePressed = false;
bool DebugCollisionsIncreaseShapeTypePressed = false;
bool DebugCollisionsIncreaseShapePressed = false;

glm::vec3 AdamPosition(-1.0f, 0.0f, -4.0f);
f32 AdamYaw = 0.0f;
i32 AdamMovementState = 0;

#define DEBUG_TIMING_AVG_SAMPLES 10

int
main(int Argc, char *Argv[])
{
    glm::quat Q {};
    Q.w = 1.0f;
    Q.x = 0.0f;
    Q.y = 0.0f;
    Q.z = 0.0f;
    glm::mat4 Test = glm::mat4_cast(Q);
    Q = Q * Q;
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

                font_info *FontContrailOne24 =
                    RasterizeAndProcessFont("resources/fonts/ContrailOne-Regular.ttf", 24);

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

                u32 DebugCollisionsShader =
                    BuildShaderProgram("resources/shaders/DebugCollisions.vs",
                                       "resources/shaders/DebugCollisions.fs");

                u32 DebugDrawShader =
                    BuildShaderProgram("resources/shaders/DebugDraw.vs",
                                       "resources/shaders/DebugDraw.fs");

                // Load models
                // -----------
                model SnowmanModel = LoadModel("resources/models/snowman/snowman.objm", true);
                model ContainerModel = LoadModel("resources/models/container/container.objm", true);
                model FloorModel = LoadModel("resources/models/primitives/floor.gltf", true);
                FloorModel.Meshes[0].DiffuseMapID = LoadTexture("resources/textures/grass.jpg", true);
                model WallModel = LoadModel("resources/models/primitives/quad.gltf", true);
                WallModel.Meshes[0].DiffuseMapID = LoadTexture("resources/textures/brickwall.jpg", true);
                WallModel.Meshes[0].NormalMapID = LoadTexture("resources/textures/brickwall_normal.jpg", true);
                skinned_model AdamModel = LoadSkinnedModel("resources/models/adam/adam.gltf", false);
                AdamModel.AnimationState.CurrentAnimationA = 2;
                AdamModel.AnimationState.CurrentAnimationB = 3;
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
                
                SetUniformInt(BasicTextShader, "FontAtlas", true, 0);

                // Light configuration
                // -------------------
                glm::vec3 LightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -0.33f));
                SetUniformVec3F(StaticMeshShader, "LightDirection", true, &LightDir[0]);
                SetUniformVec3F(SkinnedMeshShader, "LightDirection", true, &LightDir[0]);

                // Debug UI setup
                // --------------
                i32 DebugUI_X = 10;
                i32 DebugUI_Y = 10;
                char DebugUI_GLInfoBuffer[256];
                sprintf_s(DebugUI_GLInfoBuffer, "OpenGL %s (%s)",
                          glGetString(GL_VERSION),
                          glGetString(GL_RENDERER));
                ui_string DebugUI_GLInfo = PrepareUIString(DebugUI_GLInfoBuffer, FontContrailOne24,
                                                           DebugUI_X, DebugUI_Y, SCREEN_WIDTH, SCREEN_HEIGHT);

                i32 DebugUI_Line2_Y;
                CalculateUIStringOffsetPosition(DebugUI_X, DebugUI_Y, NULL, 1, FontContrailOne24,
                                                NULL, &DebugUI_Line2_Y);
                char DebugUI_FPSCounterTextBuffer[] = "FPS: ";
                ui_string DebugUI_FPSCounterText = PrepareUIString(DebugUI_FPSCounterTextBuffer, FontContrailOne24,
                                                                   DebugUI_X, DebugUI_Line2_Y, SCREEN_WIDTH, SCREEN_HEIGHT);
                i32 DebugUI_FPSCounterNumber_X;
                CalculateUIStringOffsetPosition(DebugUI_X, DebugUI_Y, DebugUI_FPSCounterTextBuffer, 1, FontContrailOne24,
                                                &DebugUI_FPSCounterNumber_X, NULL);
                char DebugUI_FPSCounterNumberBuffer[] = "000.00  ";
                ui_string DebugUI_FPSCounterNumber = PrepareUIString(DebugUI_FPSCounterNumberBuffer, FontContrailOne24,
                                                                     DebugUI_FPSCounterNumber_X, DebugUI_Line2_Y, SCREEN_WIDTH, SCREEN_HEIGHT);
                i32 DebugUI_DeltaTimeText_X;
                CalculateUIStringOffsetPosition(DebugUI_FPSCounterNumber_X, DebugUI_Line2_Y, DebugUI_FPSCounterNumberBuffer, 0, FontContrailOne24,
                                                &DebugUI_DeltaTimeText_X, NULL);
                char DebugUI_DeltaTimeTextBuffer[] = "| MS: ";
                ui_string DebugUI_DeltaTimeText = PrepareUIString(DebugUI_DeltaTimeTextBuffer, FontContrailOne24,
                                                                  DebugUI_DeltaTimeText_X, DebugUI_Line2_Y, SCREEN_WIDTH, SCREEN_HEIGHT);
                i32 DebugUI_DeltaTimeNumber_X;
                CalculateUIStringOffsetPosition(DebugUI_DeltaTimeText_X, DebugUI_Line2_Y, DebugUI_DeltaTimeTextBuffer, 0, FontContrailOne24,
                                                &DebugUI_DeltaTimeNumber_X, NULL);
                char DebugUI_DeltaTimeNumberBuffer[] = "000.00  ";
                ui_string DebugUI_DeltaTimeNumber = PrepareUIString(DebugUI_DeltaTimeNumberBuffer, FontContrailOne24,
                                                                     DebugUI_DeltaTimeNumber_X, DebugUI_Line2_Y, SCREEN_WIDTH, SCREEN_HEIGHT);
                i32 DebugUI_Other_Y;
                CalculateUIStringOffsetPosition(DebugUI_X, DebugUI_Y, NULL, 2, FontContrailOne24,
                                                NULL, &DebugUI_Other_Y);
                DEBUG_InitializeDebugUI(DebugUI_X, DebugUI_Other_Y, SCREEN_WIDTH, SCREEN_HEIGHT, BasicTextShader);
                
                DEBUG_CollisionTestSetup(DebugCollisionsShader);

                // Timing data
                // -----------
                u64 PerfCounterFrequency = SDL_GetPerformanceFrequency();
                u64 LastCounter = SDL_GetPerformanceCounter();
                f64 PrevFrameDeltaTimeSec = 0.0f;
                f64 FPS = 0.0f;
                i32 CurrentTimingSample = 0;
                f64 DeltaTimeSamples[DEBUG_TIMING_AVG_SAMPLES] = {};
                f64 FPSSamples[DEBUG_TIMING_AVG_SAMPLES] = {};
                // TODO: Remove this temp variable which is used for testing animations
                f64 ElapsedTime = 0.0f;

                // Game Loop
                // ---------

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
                        AdamYaw += (f32) PrevFrameDeltaTimeSec * 180.0f;
                        if (AdamYaw > 360.0f)
                        {
                            AdamYaw -= 360.0f;
                        }
                    }
                    if (CurrentKeyStates[SDL_SCANCODE_U])
                    {
                        AdamYaw -= (f32) PrevFrameDeltaTimeSec * 180.0f;
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
                    if (CurrentKeyStates[SDL_SCANCODE_F3] && !DebugUIToggleButtonPressed)
                    {
                        DebugUIToggleButtonPressed = true;
                        DebugUIToggle = !DebugUIToggle;
                    }
                    else if (!CurrentKeyStates[SDL_SCANCODE_F3])
                    {
                        DebugUIToggleButtonPressed = false;
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
                        glm::vec3 PositionDelta = NormalizedVelocity * CameraMovementSpeed * (f32) PrevFrameDeltaTimeSec;
                        CameraPosition += PositionDelta;
                    }

                    glm::vec3 PlayerCubeVelocity(0.0f);
                    if (CurrentKeyStates[SDL_SCANCODE_UP])
                    {
                        PlayerCubeVelocity.z = -1.0f;
                    }
                    if (CurrentKeyStates[SDL_SCANCODE_DOWN])
                    {
                        PlayerCubeVelocity.z = 1.0f;
                    }
                    if (CurrentKeyStates[SDL_SCANCODE_LEFT])
                    {
                        PlayerCubeVelocity.x = -1.0f;
                    }
                    if (CurrentKeyStates[SDL_SCANCODE_RIGHT])
                    {
                        PlayerCubeVelocity.x = 1.0f;
                    }
                    if (CurrentKeyStates[SDL_SCANCODE_PAGEUP])
                    {
                        PlayerCubeVelocity.y = 1.0f;
                    }
                    if (CurrentKeyStates[SDL_SCANCODE_PAGEDOWN])
                    {
                        PlayerCubeVelocity.y = -1.0f;
                    }
                    bool DebugCollisionsForceResolve = false;
                    if (CurrentKeyStates[SDL_SCANCODE_BACKSPACE] && !DebugCollisionsForceResolvePressed)
                    {
                        DebugCollisionsForceResolve = true;
                        DebugCollisionsForceResolvePressed = true;
                    }
                    else if (!CurrentKeyStates[SDL_SCANCODE_BACKSPACE])
                    {
                        DebugCollisionsForceResolvePressed = false;
                    }
                    bool DebugCollisionsIncreaseShapeType = false;
                    if (CurrentKeyStates[SDL_SCANCODE_LEFTBRACKET] && !DebugCollisionsIncreaseShapeTypePressed)
                    {
                        DebugCollisionsIncreaseShapeType = true;
                        DebugCollisionsIncreaseShapeTypePressed = true;
                    }
                    else if (!CurrentKeyStates[SDL_SCANCODE_LEFTBRACKET])
                    {
                        DebugCollisionsIncreaseShapeTypePressed = false;
                    }
                    bool DebugCollisionsIncreaseShape = false;
                    if (CurrentKeyStates[SDL_SCANCODE_RIGHTBRACKET] && !DebugCollisionsIncreaseShapePressed)
                    {
                        DebugCollisionsIncreaseShape = true;
                        DebugCollisionsIncreaseShapePressed = true;
                    }
                    else if (!CurrentKeyStates[SDL_SCANCODE_RIGHTBRACKET])
                    {
                        DebugCollisionsIncreaseShapePressed = false;
                    }

                    // Render
                    // ------
                    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    // Per-frame shader uniforms
                    // -------------------------

                    // Common transform matrices
                    glm::mat4 ProjectionTransform = glm::perspective(glm::radians(CameraFov), (f32)SCREEN_WIDTH / (f32)SCREEN_HEIGHT, 0.1f, 1000.0f);
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
                    ModelTransform = glm::rotate(ModelTransform, (f32) ElapsedTime, glm::vec3(0.0f, 1.0f, 0.0f));
                    ModelTransform = glm::scale(ModelTransform, glm::vec3(1.0f));
                    SetUniformMat4F(StaticMeshShader, "Model", false, glm::value_ptr(ModelTransform));
                    //RenderModel(&ContainerModel, StaticMeshShader);
                    // container 2
                    ModelTransform = glm::mat4(1.0f);
                    ModelTransform = glm::translate(ModelTransform, glm::vec3(-1.5f, 2.0f, -2.0f));
                    ModelTransform = glm::scale(ModelTransform, glm::vec3(0.70f));
                    SetUniformMat4F(StaticMeshShader, "Model", false, glm::value_ptr(ModelTransform));
                    RenderModel(&ContainerModel, StaticMeshShader);
                    // quad wall
                    ModelTransform = glm::mat4(1.0f);
                    ModelTransform = glm::translate(ModelTransform, glm::vec3(-10.0f, 0.0f, 0.0f));
                    ModelTransform = glm::rotate(ModelTransform, (f32) ElapsedTime, glm::vec3(0.0f, 1.0f, 0.0f));
                    SetUniformMat4F(StaticMeshShader, "Model", false, glm::value_ptr(ModelTransform));
                    RenderModel(&WallModel, StaticMeshShader);
                    // other side of wall (no z-fighting because faces are culled)
                    ModelTransform = glm::rotate(ModelTransform, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    SetUniformMat4F(StaticMeshShader, "Model", false, glm::value_ptr(ModelTransform));
                    RenderModel(&WallModel, StaticMeshShader);
                    // snowman
                    ModelTransform = glm::mat4(1.0f);
                    ModelTransform = glm::translate(ModelTransform, glm::vec3(0.0f, 0.0f, -5.0f));
                    ModelTransform = glm::rotate(ModelTransform, (f32) ElapsedTime * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    SetUniformMat4F(StaticMeshShader, "Model", false, glm::value_ptr(ModelTransform));
                    RenderModel(&SnowmanModel, StaticMeshShader);
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

                        AdamPositionDelta = Direction * Velocity * (f32) PrevFrameDeltaTimeSec;
                    }
                    AdamPosition += AdamPositionDelta;
                    ModelTransform = glm::translate(ModelTransform, AdamPosition);
                    ModelTransform = glm::rotate(ModelTransform, glm::radians(AdamYaw), glm::vec3(0.0f, 1.0f, 0.0f));
                    //ModelTransform = glm::scale(ModelTransform, glm::vec3(0.5f));
                    SetUniformMat4F(SkinnedMeshShader, "Model", true, glm::value_ptr(ModelTransform));
                    RenderSkinnedModel(&AdamModel, SkinnedMeshShader, (f32) PrevFrameDeltaTimeSec);

                    DEBUG_CollisionTestUpdate(DebugCollisionsShader, DebugDrawShader,
                                              (f32) PrevFrameDeltaTimeSec,
                                              ProjectionTransform, ViewTransform,
                                              PlayerCubeVelocity,
                                              DebugCollisionsForceResolve,
                                              DebugCollisionsIncreaseShapeType,
                                              DebugCollisionsIncreaseShape);

                    // Render Debug UI
                    // ---------------

                    if (DebugUIToggle)
                    {
                        UseShader(BasicTextShader);

                        if (CurrentTimingSample == 0)
                        {
                            f64 FPSAverage = 0.0f;
                            f64 DeltaTimeAverage = 0.0f;
                            for (i32 Index = 0; Index < DEBUG_TIMING_AVG_SAMPLES; ++Index)
                            {
                                FPSAverage += FPSSamples[Index];
                                DeltaTimeAverage += DeltaTimeSamples[Index];
                            }
                            FPSAverage = FPSAverage / (f64) DEBUG_TIMING_AVG_SAMPLES;
                            DeltaTimeAverage = DeltaTimeAverage / (f64) DEBUG_TIMING_AVG_SAMPLES;

                            sprintf_s(DebugUI_FPSCounterNumberBuffer, "%.2f", FPSAverage);
                            UpdateUIString(DebugUI_FPSCounterNumber, DebugUI_FPSCounterNumberBuffer);
                            sprintf_s(DebugUI_DeltaTimeNumberBuffer, "%.2f", DeltaTimeAverage * 1000.0);
                            UpdateUIString(DebugUI_DeltaTimeNumber, DebugUI_DeltaTimeNumberBuffer);
                        }

                        RenderUIString(DebugUI_GLInfo);
                        RenderUIString(DebugUI_FPSCounterText);
                        RenderUIString(DebugUI_FPSCounterNumber);
                        RenderUIString(DebugUI_DeltaTimeText);
                        RenderUIString(DebugUI_DeltaTimeNumber);

                        DEBUG_RenderAllDebugStrings();

                        UseShader(0);
                    }

                    DEBUG_ResetAllDebugStrings();

                    // Swap buffer
                    // -----------
                    SDL_GL_SwapWindow(Window);

                    // Timing
                    // ------
                    u64 CurrentCounter = SDL_GetPerformanceCounter();
                    u64 CounterElapsed = CurrentCounter - LastCounter;
                    LastCounter = CurrentCounter;
                    PrevFrameDeltaTimeSec = (f64) CounterElapsed / (f64) PerfCounterFrequency;
                    FPS = (f64) PerfCounterFrequency / (f64) CounterElapsed;
                    DeltaTimeSamples[CurrentTimingSample] = PrevFrameDeltaTimeSec;
                    FPSSamples[CurrentTimingSample] = FPS;
                    ++CurrentTimingSample;
                    if (CurrentTimingSample >= DEBUG_TIMING_AVG_SAMPLES)
                    {
                        CurrentTimingSample = 0;
                    }
                    ElapsedTime += PrevFrameDeltaTimeSec;
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
