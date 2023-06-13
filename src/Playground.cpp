#include <glad/glad.h>
#include <sdl2/SDL.h>
#include <sdl2/SDL_image.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "Common.h"

const u32 SCREEN_WIDTH = 1280;
const u32 SCREEN_HEIGHT = 720;

std::string readFile(const char *path)
{
    std::string content;
    std::ifstream fileStream;
    fileStream.exceptions(std::ifstream::failbit | std::ifstream:: badbit);
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
                // GLAD: GL function pointers
                // --------------------------
                gladLoadGLLoader(SDL_GL_GetProcAddress);
                std::cout << "OpenGL loaded\n";
                std::cout << "Vendor: " <<  glGetString(GL_VENDOR) << '\n';
                std::cout << "Renderer: " <<  glGetString(GL_RENDERER) << '\n';
                std::cout << "Version: " << glGetString(GL_VERSION) << '\n';

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
                glShaderSource(vertexShader,1 , &vertexShaderSourceCStr, NULL);
                glCompileShader(vertexShader);
                int success;
                char infoLog[512];
                glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
                if (!success)
                {
                    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
                    std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << '\n';
                }
                u32 fragmentShader;
                fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
                std::string fragmentShaderSource = readFile("resources/shaders/Basic.fs");
                const char *fragmentShaderSourceCStr = fragmentShaderSource.c_str();
                glShaderSource(fragmentShader,1 , &fragmentShaderSourceCStr, NULL);
                glCompileShader(fragmentShader);
                glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
                if (!success)
                {
                    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
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
                    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
                    std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << '\n';
                }
                glDeleteShader(vertexShader);
                glDeleteShader(fragmentShader);

                // Vertex data
                // -----------
                f32 vertices[] = {
                    // positions        // colors         // uv
                    -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                     0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
                     0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f
                };
                u32 VBO;
                glGenBuffers(1, &VBO);
                u32 VAO;
                glGenVertexArrays(1, &VAO);
                glBindVertexArray(VAO);
                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void *)0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void *)(3 * sizeof(f32)));
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void *)(6 * sizeof(f32)));
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);

                // Load textures
                int imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
                if (!(IMG_Init(imgFlags) & imgFlags))
                {
                    std::cerr << "SDL_image could not initialize: " << IMG_GetError() << '\n';
                    SDL_Quit();
                    exit(-1);
                }

                u32 textureID = 0;
                SDL_Surface *texture = IMG_Load("resources/textures/container.jpg");
                if (texture)
                {
                    GLenum format = GL_RGB;
                    if (texture->format->BytesPerPixel == 4)
                    {
                        format = GL_RGBA;
                    }

                    glGenTextures(1, &textureID);
                    glBindTexture(GL_TEXTURE_2D, textureID);
                    glTexImage2D(GL_TEXTURE_2D, 0, format, texture->w, texture->h, 0, format, GL_UNSIGNED_BYTE, texture->pixels);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                else
                {
                    std::cerr << "Failed to load texture\n";
                }

                glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);

                // Game Loop
                // ---------
                SDL_Event event;
                bool quit = false;
                while (!quit)
                {
                    // Render
                    // ------
                    glClearColor(1.0f, 0.3f, 0.3f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT);

                    glUseProgram(shaderProgram);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, textureID);
                    glBindVertexArray(VAO);
                    glDrawArrays(GL_TRIANGLES, 0, 3);
                    glBindVertexArray(0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glUseProgram(0);

                    // Swap buffer
                    // -----------
                    SDL_GL_SwapWindow(window);

                    // Poll SDL events
                    // ---------------
                    while (SDL_PollEvent(&event))
                    {
                        if (event.type == SDL_QUIT)
                        {
                            quit = true;
                        }
                        else if (event.type == SDL_KEYDOWN)
                        {
                            SDL_KeyboardEvent *keyboardEvent = &event.key;

                            if (keyboardEvent->keysym.sym == SDLK_ESCAPE && keyboardEvent->state == SDL_PRESSED)
                            {
                                quit = true;
                            }
                        }
                    }
                }
                
            }
            else
            {
                std::cerr << "Couldn't create OpenGL context: " <<  SDL_GetError() << '\n';
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