#include "DebugUI.h"

#include <glad/glad.h>
#include <cstring>

#include "Shader.h"
#include "Text.h"
#include "Util.h"

#define DEBUG_STRING_MAX_LENGTH 128
struct debug_string
{
    char Buffer[DEBUG_STRING_MAX_LENGTH];
    size_t Length;
    size_t BufferSize = DEBUG_STRING_MAX_LENGTH;
};

#define DEBUG_STRING_MAX_COUNT 32
debug_string gDebugStrings[DEBUG_STRING_MAX_COUNT];
i32 gDebugStringCount = 0;

font_info *gFont;

i32 gX;
i32 gY;
i32 gScreenWidth;
i32 gScreenHeight;

u32 gShader;
u32 gVAO;
u32 gVBO;
size_t gVertCount;
f32 *gVertPosBuffer;
f32 *gVertUVBuffer;

void
DEBUG_InitializeDebugUI(i32 X, i32 Y, i32 ScreenWidth, i32 ScreenHeight, u32 TextShader)
{
    // 1. Save screen info
    // -------------------
    gX = X;
    gY = Y;
    gScreenWidth = ScreenWidth;
    gScreenHeight = ScreenHeight;
    gShader = TextShader;
    
    // 2. Load font
    // ------------
    gFont = RasterizeAndProcessFont("resources/fonts/ContrailOne-Regular.ttf", 24);

    // 3. Allocate buffers for transient render data
    // ---------------------------------------------
    size_t BufferSize = DEBUG_STRING_MAX_LENGTH;
    gVertCount = BufferSize * 6;
    gVertPosBuffer = (f32 *) malloc(gVertCount * 2 * sizeof(f32));
    gVertUVBuffer = (f32 *) malloc(gVertCount * 2 * sizeof(f32));

    // 4. Prepare render data on GPU
    // -----------------------------
    glGenVertexArrays(1, &gVAO);
    glGenBuffers(1, &gVBO);
    glBindVertexArray(gVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    glBufferData(GL_ARRAY_BUFFER, (2 + 2) * gVertCount * sizeof(f32), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void *) 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void *) (2 * gVertCount * sizeof(f32)));
    glBindVertexArray(0);
}

void
DEBUG_AddDebugString(const char *String)
{
    Assert(gDebugStringCount < DEBUG_STRING_MAX_COUNT);
    debug_string *DebugString = &gDebugStrings[gDebugStringCount];
    DebugString->BufferSize = DEBUG_STRING_MAX_LENGTH;
    size_t Length = GetNullTerminatedStringLength(String);
    if (Length + 1 > DebugString->BufferSize)
    {
        Length = DebugString->BufferSize - 1;
    }
    strncpy_s(DebugString->Buffer, String, DebugString->BufferSize - 1);
    DebugString->Length = Length;

    gDebugStringCount++;
}

void
DEBUG_RenderAllDebugStrings()
{
    if (gDebugStringCount > 0)
    {
        UseShader(gShader);

        glBindVertexArray(gVAO);
        glBindBuffer(GL_ARRAY_BUFFER, gVBO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gFont->AtlasGLID);

        for (i32 StringIndex = 0; StringIndex < gDebugStringCount; ++StringIndex)
        {
            debug_string *DebugString = &gDebugStrings[StringIndex];

            PrepareRenderDataForString(DebugString->Buffer, (i32) DebugString->Length, (i32) DebugString->Length, gFont,
                                       gX, gY, gScreenWidth, gScreenHeight, StringIndex, gVertPosBuffer, gVertUVBuffer);

            glBufferSubData(GL_ARRAY_BUFFER, 0,
                            DebugString->Length * 6 * 2 * sizeof(f32), gVertPosBuffer);
            glBufferSubData(GL_ARRAY_BUFFER, 2 * gVertCount * sizeof(f32),
                            DebugString->Length * 6 * 2 * sizeof(f32), gVertUVBuffer);

            glDrawArrays(GL_TRIANGLES, 0, (i32) DebugString->Length * 6);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

void
DEBUG_ResetAllDebugStrings()
{
    gDebugStringCount = 0;
}
