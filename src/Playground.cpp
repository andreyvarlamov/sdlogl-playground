#include <glad/glad.h>
#include <sdl2/SDL.h>

#include <cstdio>

#include "Common.h"

const u32 SCREEN_WIDTH = 1280;
const u32 SCREEN_HEIGHT = 720;

const char *vertexShaderSource = 
	"#version 330 core\n"
	"layout (location = 0) in vec3 aPos;\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
	"}\0";

const char *fragmentShaderSource =
	"#version 330 core\n"
	"out vec4 FragColor;\n"
	"void main()\n"
	"{\n"
	"    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
	"}\0";


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
                printf("OpenGL loaded\n");
                printf("Vendor: %s\n", glGetString(GL_VENDOR));
                printf("Renderer: %s\n", glGetString(GL_RENDERER));
                printf("Version: %s\n", glGetString(GL_VERSION));

                int windowWidth;
                int windowHeight;
                SDL_GetWindowSize(window, &windowWidth, &windowHeight);
                glViewport(0, 0, windowWidth, windowHeight);

                // Load shaders
                // ------------
                u32 vertexShader;
                vertexShader = glCreateShader(GL_VERTEX_SHADER);
                glShaderSource(vertexShader,1 , &vertexShaderSource, NULL);
                glCompileShader(vertexShader);
                int success;
                char infoLog[512];
                glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
                if (!success)
                {
                    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
                    fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
                }
                u32 fragmentShader;
                fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
                glShaderSource(fragmentShader,1 , &fragmentShaderSource, NULL);
                glCompileShader(fragmentShader);
                glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
                if (!success)
                {
                    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
                    fprintf(stderr, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
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
                    fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
                }
                glDeleteShader(vertexShader);
                glDeleteShader(fragmentShader);

                // Vertex data
                // -----------
                f32 vertices[] = {
                    -0.5f, -0.5f, 0.0f,
                     0.5f, -0.5f, 0.0f,
                     0.0f,  0.5f, 0.0f
                };
                u32 VBO;
                glGenBuffers(1, &VBO);
                u32 VAO;
                glGenVertexArrays(1, &VAO);
                glBindVertexArray(VAO);
                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (void *)0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);

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
                    glBindVertexArray(VAO);
                    glDrawArrays(GL_TRIANGLES, 0, 3);
                    glBindVertexArray(0);
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
                fprintf(stderr, "Couldn't create OpenGL context: %s\n", SDL_GetError());
            }
        }
        else
        {
            fprintf(stderr, "Couldn't create SDL window: %s\n", SDL_GetError());
        }

    }
    else
    {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
    }

    SDL_Quit();
    return 0;
}