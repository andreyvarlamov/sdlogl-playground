#include <glad/glad.h>
#include <sdl2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
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

std::string readFile(const char *path);
u32 loadTexture(const char* path);
std::vector<Mesh> loadModel(const char *path);
glm::vec3 calculateTangentForTriangle(const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs);
void generateTriangleVertexData(f32 *vertexData, const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs);
void generatePlaneVertexData(f32 *vertexData, const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs);
u32 prepareQuadVAO(const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs);
u32 prepareCubeVAO();
u32 prepareMeshVAO(f32 *vertexData, u32 vertexCount, u32 *indices, u32 indexCount);

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
                u32 vertexShader;
                vertexShader = glCreateShader(GL_VERTEX_SHADER);
                // TODO: Implement custom memory alloc + custom string
                std::string vertexShaderSource = readFile("resources/shaders/Basic.vs");
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
                // TODO: Implement custom memory alloc + custom string
                std::string fragmentShaderSource = readFile("resources/shaders/Basic.fs");
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
                std::vector<Mesh> snowmanMeshes = loadModel("resources/models/snowman/snowman.objm");
                std::vector<Mesh> backpackMeshes = loadModel("resources/models/backpack/backpack.objm");

                // Shader global uniforms
                // ----------------------
                glUseProgram(shaderProgram);
                glUniform1i(glGetUniformLocation(shaderProgram, "diffuseMap"), 0);
                glUseProgram(shaderProgram);
                glUniform1i(glGetUniformLocation(shaderProgram, "specularMap"), 1);
                glUseProgram(shaderProgram);
                glUniform1i(glGetUniformLocation(shaderProgram, "emissionMap"), 2);
                glUseProgram(shaderProgram);
                glUniform1i(glGetUniformLocation(shaderProgram, "normalMap"), 3);

                // Light configuration
                // -------------------
                glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -0.33f));
                glUniform3fv(glGetUniformLocation(shaderProgram, "lightDirection"), 1, &lightDir[0]);

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

                    glUseProgram(shaderProgram);

                    // perspective projection
                    glm::mat4 projection = glm::perspective(glm::radians(CameraFov / 2.0f), (f32)SCREEN_WIDTH / (f32)SCREEN_HEIGHT, 0.1f, 100.0f);
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

                    // camera view matrix
                    glm::mat4 view = glm::lookAt(CameraPosition, CameraPosition + CameraFront, CameraUp);
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

                    // view position for light calculation
                    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPosition"), 1, &CameraPosition[0]);

                    glm::mat4 model = glm::mat4(1.0f);
                    glm::mat3 normalMatrix = glm::mat3(1.0f);

                    // floor
                    model = glm::mat4(1.0f);
                    model = glm::scale(model, glm::vec3(25.0f));
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, grassTextureID);
                    glBindVertexArray(floorVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                    glBindVertexArray(0);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);

                    // cube 1
                    model = glm::mat4(1.0f);
                    model = glm::rotate(model, currentFrame / 1000.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(1.0f));
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    glBindVertexArray(cubeVAO);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, containerDiffuseID);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, containerSpecularID);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, eyeEmissionID);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glBindVertexArray(0);

                    // cube 2
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(-1.5f, 2.0f, -2.0f));
                    model = glm::scale(model, glm::vec3(0.70f));
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    glBindVertexArray(cubeVAO);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, containerDiffuseID);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, containerSpecularID);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glBindVertexArray(0);

                    // quad wall
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(-5.0f, 0.0f, 0.0f));
                    model = glm::rotate(model, currentFrame / 1000.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    glBindVertexArray(wallVAO);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallDiffuseID);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, wallNormalTexID);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                    // other side of wall (no z-fighting because faces are culled)
                    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glBindVertexArray(0);

                    // snowman
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(5.0f, 0.0f, 0.0f));
                    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    for (u32 snowmanMeshIndex = 0; snowmanMeshIndex < snowmanMeshes.size(); ++snowmanMeshIndex)
                    {
                        Mesh mesh = snowmanMeshes[snowmanMeshIndex];
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, mesh.diffuseMapID);
                        glBindVertexArray(mesh.vao);
                        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
                        glBindVertexArray(0);
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE0, 0);
                    }

                    // backpack
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(0.0f, 2.0f, 5.0f));
                    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(0.5f));
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    for (u32 backpackMeshIndex = 0; backpackMeshIndex < backpackMeshes.size(); ++backpackMeshIndex)
                    {
                        Mesh mesh = backpackMeshes[backpackMeshIndex];
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, mesh.diffuseMapID);
                        glBindVertexArray(mesh.vao);
                        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
                        glBindVertexArray(0);
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE0, 0);
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
    // TODO: Implement custom memory alloc + custom string
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

std::vector<Mesh> loadModel(const char *path)
{
    // TODO: Custom memory allocation
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


    for (u32 meshIndex = 0; meshIndex < assimpScene->mNumMeshes; ++meshIndex)
    {
        std::cout << "Loading mesh #" << meshIndex << "/" << assimpScene->mNumMeshes << '\n';
        Mesh mesh;

        aiMesh *assimpMesh = assimpScene->mMeshes[meshIndex];

        f32 *meshVertexData;
        size_t meshVertexFloatCount;
        u32 *meshIndices;
        size_t meshIndexCount;
        // Position + Normal + UVs + Tangent + Bitangent
        size_t perVertexFloatCount = (3 + 3 + 2 + 3 + 3);
        u32 assimpVertexCount = assimpMesh->mNumVertices;
        meshVertexFloatCount = perVertexFloatCount * assimpVertexCount;
        // TODO: Custom memory allocation
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
        
        // TODO: Custom memory allocation
        free(meshVertexData);
        free(meshIndices);

        aiMaterial *mat = assimpScene->mMaterials[assimpMesh->mMaterialIndex];
        u32 textureCount = aiGetMaterialTextureCount(mat, aiTextureType_DIFFUSE);
        if (textureCount > 0)
        {
            aiString assimpTextureFileName;
            aiGetMaterialString(mat, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), &assimpTextureFileName);

            // TODO: custom strings
            std::string modelPath = std::string(path);
            std::string modelDirectory = modelPath.substr(0, modelPath.find_last_of('/'));
            // TODO: this is using assimp C++ interface
            std::string textureFileName = std::string(assimpTextureFileName.C_Str());
            std::string texturePath = modelDirectory + '/' + textureFileName;
            std::cout << "T Start ... ";
            // TODO: Need caching
            // E.g. this is loading the same 5 MB texture 78 times for 78 different meshes....
            mesh.diffuseMapID = loadTexture(texturePath.c_str());
            std::cout << "End" << '\n';
        }
        // TODO: specular
        // TODO: emission
        // TODO: normal

        meshes.push_back(mesh);
    }

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

u32 prepareMeshVAO(f32 *vertexData, u32 vertexCount, u32 *indices, u32 indexCount)
{
    u32 meshVAO;
    glGenVertexArrays(1, &meshVAO);
    u32 meshVBO;
    glGenBuffers(1, &meshVBO);
    u32 meshEBO;
    glGenBuffers(1, &meshEBO);

    glBindVertexArray(meshVAO);
    glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(f32), vertexData, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(u32), indices, GL_STATIC_DRAW);

    // Position + Normal + UVs + Tangent + Bitangent
    size_t vertexSize = (3 + 3 + 2 + 3 + 3) * sizeof(f32);

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
