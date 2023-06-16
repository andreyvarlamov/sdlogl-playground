#include <glad/glad.h>
#include <sdl2/SDL.h>
#include <sdl2/SDL_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

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

glm::vec3 calculateTangentForTriangle(const glm::vec3 *positions, glm::vec3 normal, const glm::vec2 *uvs)
{
    glm::vec3 result;

    glm::vec3 edge1 = positions[1] - positions[0];
    glm::vec3 edge2 = positions[2] - positions[0];
    glm::vec2 deltaUV1 = uvs[1] - uvs[0];
    glm::vec2 deltaUV2 = uvs[2] - uvs[0];
    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
    result.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    result.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    result.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

    return result;
}

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
                f32 cubeVertices[] = {
                    // position           // normals          // uv
                    // back face
                    -0.5f,  0.0f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                     0.5f,  1.0f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                     0.5f,  0.0f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
                     0.5f,  1.0f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                    -0.5f,  0.0f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                    -0.5f,  1.0f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
                    // front face
                    -0.5f,  0.0f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                     0.5f,  0.0f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
                     0.5f,  1.0f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                     0.5f,  1.0f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                    -0.5f,  1.0f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
                    -0.5f,  0.0f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                    // left face
                    -0.5f,  1.0f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                    -0.5f,  1.0f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
                    -0.5f,  0.0f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                    -0.5f,  0.0f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                    -0.5f,  0.0f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                    -0.5f,  1.0f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                    // right face
                     0.5f,  1.0f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                     0.5f,  0.0f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                     0.5f,  1.0f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
                     0.5f,  0.0f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                     0.5f,  1.0f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                     0.5f,  0.0f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
                    // bottom face
                    -0.5f,  0.0f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                     0.5f,  0.0f, -0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
                     0.5f,  0.0f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                     0.5f,  0.0f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                    -0.5f,  0.0f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                    -0.5f,  0.0f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                    // top face
                    -0.5f,  1.0f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                     0.5f,  1.0f , 0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                     0.5f,  1.0f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
                     0.5f,  1.0f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                    -0.5f,  1.0f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                    -0.5f,  1.0f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
                };
                u32 cubeVBO;
                glGenBuffers(1, &cubeVBO);
                u32 cubeVAO;
                glGenVertexArrays(1, &cubeVAO);
                glBindVertexArray(cubeVAO);
                glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void *)0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void *)(3 * sizeof(f32)));
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void *)(6 * sizeof(f32)));
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);

                // floor plane
                glm::vec3 floorTriangle1Positions[] = {
                    glm::vec3(1.0f, 0.0f, 1.0f),
                    glm::vec3(-1.0f, 0.0f, -1.0f),
                    glm::vec3(-1.0f, 0.0f, 1.0f)
                };
                glm::vec3 floorNormal(0.0f, 1.0f, 0.0f);
                glm::vec2 floorTriangle1Uvs[] = {
                    glm::vec2(25.0f, 0.0f),
                    glm::vec2(0.0f, 25.0f),
                    glm::vec2(0.0f, 0.0f)
                };
                glm::vec3 floorTriangle1Tangent = calculateTangentForTriangle(floorTriangle1Positions, floorNormal, floorTriangle1Uvs);
                glm::vec3 floorTriangle2Positions[] = {
                    glm::vec3(1.0f, 0.0f, 1.0f),
                    glm::vec3(1.0f, 0.0f, -1.0f),
                    glm::vec3(-1.0f, 0.0f, -1.0f)
                };
                glm::vec2 floorTriangle2Uvs[] = {
                    glm::vec2(25.0f, 0.0f),
                    glm::vec2(25.0f, 25.0f),
                    glm::vec2(0.0f, 25.0f)
                };
                glm::vec3 floorTriangle2Tangent = calculateTangentForTriangle(floorTriangle2Positions, floorNormal, floorTriangle2Uvs);
                float floorVertices[] = {
                    // triangle 1
                    floorTriangle1Positions[0].x, floorTriangle1Positions[0].y, floorTriangle1Positions[0].z,
                    floorNormal.x, floorNormal.y, floorNormal.z,
                    floorTriangle1Uvs[0].x, floorTriangle1Uvs[0].y,
                    floorTriangle1Tangent.x, floorTriangle1Tangent.y, floorTriangle1Tangent.z,

                    floorTriangle1Positions[1].x, floorTriangle1Positions[1].y, floorTriangle1Positions[1].z,
                    floorNormal.x, floorNormal.y, floorNormal.z,
                    floorTriangle1Uvs[1].x, floorTriangle1Uvs[1].y,
                    floorTriangle1Tangent.x, floorTriangle1Tangent.y, floorTriangle1Tangent.z,

                    floorTriangle1Positions[2].x, floorTriangle1Positions[2].y, floorTriangle1Positions[2].z,
                    floorNormal.x, floorNormal.y, floorNormal.z,
                    floorTriangle1Uvs[2].x, floorTriangle1Uvs[2].y,
                    floorTriangle1Tangent.x, floorTriangle1Tangent.y, floorTriangle1Tangent.z,

                    // triangle 2
                    floorTriangle2Positions[0].x, floorTriangle2Positions[0].y, floorTriangle2Positions[0].z,
                    floorNormal.x, floorNormal.y, floorNormal.z,
                    floorTriangle2Uvs[0].x, floorTriangle2Uvs[0].y,
                    floorTriangle2Tangent.x, floorTriangle2Tangent.y, floorTriangle2Tangent.z,

                    floorTriangle2Positions[1].x, floorTriangle2Positions[1].y, floorTriangle2Positions[1].z,
                    floorNormal.x, floorNormal.y, floorNormal.z,
                    floorTriangle2Uvs[1].x, floorTriangle2Uvs[1].y,
                    floorTriangle2Tangent.x, floorTriangle2Tangent.y, floorTriangle2Tangent.z,

                    floorTriangle2Positions[2].x, floorTriangle2Positions[2].y, floorTriangle2Positions[2].z,
                    floorNormal.x, floorNormal.y, floorNormal.z,
                    floorTriangle2Uvs[2].x, floorTriangle2Uvs[2].y,
                    floorTriangle2Tangent.x, floorTriangle2Tangent.y, floorTriangle2Tangent.z
                };
                u32 floorVBO;
                glGenBuffers(1, &floorVBO);
                u32 floorVAO;
                glGenVertexArrays(1, &floorVAO);
                glBindVertexArray(floorVAO);
                glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
                glEnableVertexAttribArray(3);
                glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
                glBindVertexArray(0);

                // wall with calculated tangents
                glm::vec3 wallTriangle1Positions[] = {
                    glm::vec3(-1.0f,  2.0f, 0.0f),
                    glm::vec3(-1.0f,  0.0f, 0.0f),
                    glm::vec3( 1.0f,  0.0f, 0.0f)
                };
                glm::vec3 wallNormal(0.0f, 0.0f, 1.0f);
                glm::vec2 wallTriangle1Uvs[] = {
                    glm::vec2(0.0f, 1.0f),
                    glm::vec2(0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f)
                };
                glm::vec3 wallTriangle1Tangent = calculateTangentForTriangle(wallTriangle1Positions, wallNormal, wallTriangle1Uvs);
                glm::vec3 wallTriangle2Positions[] = {
                    glm::vec3(-1.0f,  2.0f, 0.0f),
                    glm::vec3( 1.0f,  0.0f, 0.0f),
                    glm::vec3( 1.0f,  2.0f, 0.0f)
                };
                glm::vec2 wallTriangle2Uvs[] = {
                    glm::vec2(0.0f, 1.0f),
                    glm::vec2(1.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f)
                };
                glm::vec3 wallTriangle2Tangent = calculateTangentForTriangle(wallTriangle2Positions, wallNormal, wallTriangle2Uvs);
                float wallVertices[] = {
                    // triangle 1
                    wallTriangle1Positions[0].x, wallTriangle1Positions[0].y, wallTriangle1Positions[0].z,
                    wallNormal.x, wallNormal.y, wallNormal.z,
                    wallTriangle1Uvs[0].x, wallTriangle1Uvs[0].y,
                    wallTriangle1Tangent.x, wallTriangle1Tangent.y, wallTriangle1Tangent.z,

                    wallTriangle1Positions[1].x, wallTriangle1Positions[1].y, wallTriangle1Positions[1].z,
                    wallNormal.x, wallNormal.y, wallNormal.z,
                    wallTriangle1Uvs[1].x, wallTriangle1Uvs[1].y,
                    wallTriangle1Tangent.x, wallTriangle1Tangent.y, wallTriangle1Tangent.z,

                    wallTriangle1Positions[2].x, wallTriangle1Positions[2].y, wallTriangle1Positions[2].z,
                    wallNormal.x, wallNormal.y, wallNormal.z,
                    wallTriangle1Uvs[2].x, wallTriangle1Uvs[2].y,
                    wallTriangle1Tangent.x, wallTriangle1Tangent.y, wallTriangle1Tangent.z,

                    // triangle 2
                    wallTriangle2Positions[0].x, wallTriangle2Positions[0].y, wallTriangle2Positions[0].z,
                    wallNormal.x, wallNormal.y, wallNormal.z,
                    wallTriangle2Uvs[0].x, wallTriangle2Uvs[0].y,
                    wallTriangle2Tangent.x, wallTriangle2Tangent.y, wallTriangle2Tangent.z,

                    wallTriangle2Positions[1].x, wallTriangle2Positions[1].y, wallTriangle2Positions[1].z,
                    wallNormal.x, wallNormal.y, wallNormal.z,
                    wallTriangle2Uvs[1].x, wallTriangle2Uvs[1].y,
                    wallTriangle2Tangent.x, wallTriangle2Tangent.y, wallTriangle2Tangent.z,

                    wallTriangle2Positions[2].x, wallTriangle2Positions[2].y, wallTriangle2Positions[2].z,
                    wallNormal.x, wallNormal.y, wallNormal.z,
                    wallTriangle2Uvs[2].x, wallTriangle2Uvs[2].y,
                    wallTriangle2Tangent.x, wallTriangle2Tangent.y, wallTriangle2Tangent.z
                };
                u32 wallVBO;
                glGenBuffers(1, &wallVBO);
                u32 wallVAO;
                glGenVertexArrays(1, &wallVAO);
                glBindVertexArray(wallVAO);
                glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(wallVertices), wallVertices, GL_STATIC_DRAW);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
                glEnableVertexAttribArray(3);
                glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
                glBindVertexArray(0);

                // Load textures
                int imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
                if (!(IMG_Init(imgFlags) & imgFlags))
                {
                    std::cerr << "SDL_image could not initialize: " << IMG_GetError() << '\n';
                    SDL_Quit();
                    exit(-1);
                }

                u32 containerDiffuseID = 0;
                SDL_Surface *containerDiffuse = IMG_Load("resources/textures/container.png");
                if (containerDiffuse)
                {
                    GLenum format = GL_RGB;
                    if (containerDiffuse->format->BytesPerPixel == 4)
                    {
                        format = GL_RGBA;
                    }

                    glGenTextures(1, &containerDiffuseID);
                    glBindTexture(GL_TEXTURE_2D, containerDiffuseID);
                    glTexImage2D(GL_TEXTURE_2D, 0, format, containerDiffuse->w, containerDiffuse->h, 0, format, GL_UNSIGNED_BYTE, containerDiffuse->pixels);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                else
                {
                    std::cerr << "Failed to load texture\n";
                }

                u32 containerSpecularID = 0;
                SDL_Surface *containerSpecular = IMG_Load("resources/textures/container_specular.png");
                if (containerSpecular)
                {
                    GLenum format = GL_RGB;
                    if (containerSpecular->format->BytesPerPixel == 4)
                    {
                        format = GL_RGBA;
                    }

                    glGenTextures(1, &containerSpecularID);
                    glBindTexture(GL_TEXTURE_2D, containerSpecularID);
                    glTexImage2D(GL_TEXTURE_2D, 0, format, containerSpecular->w, containerSpecular->h, 0, format, GL_UNSIGNED_BYTE, containerSpecular->pixels);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                else
                {
                    std::cerr << "Failed to load texture\n";
                }

                u32 eyeEmissionID = 0;
                SDL_Surface *eyeEmission = IMG_Load("resources/textures/eye_emission.png");
                if (eyeEmission)
                {
                    GLenum format = GL_RGB;
                    if (eyeEmission->format->BytesPerPixel == 4)
                    {
                        format = GL_RGBA;
                    }

                    glGenTextures(1, &eyeEmissionID);
                    glBindTexture(GL_TEXTURE_2D, eyeEmissionID);
                    glTexImage2D(GL_TEXTURE_2D, 0, format, eyeEmission->w, eyeEmission->h, 0, format, GL_UNSIGNED_BYTE, eyeEmission->pixels);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                else
                {
                    std::cerr << "Failed to load texture\n";
                }

                u32 wallDiffuseID = 0;
                SDL_Surface *wallDiffuse = IMG_Load("resources/textures/brickwall.jpg");
                if (wallDiffuse)
                {
                    GLenum format = GL_RGB;
                    if (wallDiffuse->format->BytesPerPixel == 4)
                    {
                        format = GL_RGBA;
                    }

                    glGenTextures(1, &wallDiffuseID);
                    glBindTexture(GL_TEXTURE_2D, wallDiffuseID);
                    glTexImage2D(GL_TEXTURE_2D, 0, format, wallDiffuse->w, wallDiffuse->h, 0, format, GL_UNSIGNED_BYTE, wallDiffuse->pixels);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                else
                {
                    std::cerr << "Failed to load texture\n";
                }

                u32 wallNormalTexID = 0;
                SDL_Surface *wallNormalTex = IMG_Load("resources/textures/brickwall_normal.jpg");
                if (wallNormalTex)
                {
                    GLenum format = GL_RGB;
                    if (wallNormalTex->format->BytesPerPixel == 4)
                    {
                        format = GL_RGBA;
                    }

                    glGenTextures(1, &wallNormalTexID);
                    glBindTexture(GL_TEXTURE_2D, wallNormalTexID);
                    glTexImage2D(GL_TEXTURE_2D, 0, format, wallNormalTex->w, wallNormalTex->h, 0, format, GL_UNSIGNED_BYTE, wallNormalTex->pixels);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                else
                {
                    std::cerr << "Failed to load texture\n";
                }

                u32 grassTextureID = 0;
                SDL_Surface *grassTexture = IMG_Load("resources/textures/grass.jpg");
                if (grassTexture)
                {
                    GLenum format = GL_RGB;
                    if (grassTexture->format->BytesPerPixel == 4)
                    {
                        format = GL_RGBA;
                    }

                    glGenTextures(1, &grassTextureID);
                    glBindTexture(GL_TEXTURE_2D, grassTextureID);
                    glTexImage2D(GL_TEXTURE_2D, 0, format, grassTexture->w, grassTexture->h, 0, format, GL_UNSIGNED_BYTE, grassTexture->pixels);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                else
                {
                    std::cerr << "Failed to load texture\n";
                }

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
                float deltaTime = 0.0f;
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
                    deltaTime = (float)((double)(currentFrame - lastFrame) / 1000.0);
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
                    glm::mat4 projection = glm::perspective(glm::radians(CameraFov / 2.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
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

                    // cube 1
                    model = glm::mat4(1.0f);
                    model = glm::rotate(model, currentFrame / 1000.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(1.0f));
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, containerDiffuseID);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, containerSpecularID);
                    glBindVertexArray(cubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                    glBindVertexArray(0);

                    // cube 2
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(-1.5f, 2.0f, -2.0f));
                    model = glm::scale(model, glm::vec3(0.70f));
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, containerDiffuseID);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, containerSpecularID);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, eyeEmissionID);
                    glBindVertexArray(cubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                    glBindVertexArray(0);

                    // quad wall
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(-5.0f, 0.0f, 0.0f));
                    model = glm::rotate(model, currentFrame / 1000.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallDiffuseID);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, wallNormalTexID);
                    glBindVertexArray(wallVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                    glBindVertexArray(0);
                    // other side of wall (no z-fighting because faces are culled)
                    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallDiffuseID);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, wallNormalTexID);
                    glBindVertexArray(wallVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                    glBindVertexArray(0);


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