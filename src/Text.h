#ifndef TEXT_H
#define TEXT_H

#include <glm/glm.hpp>

#include "Common.h"

i32
DEBUG_RasterizeFontIntoGLTexture(const char *FontPath, i32 FontSizePoints);

u32
DEBUG_PrepareRenderDataForString(i32 FontID, const char *Text, i32 TextCount,
                                 i32 XPos, i32 YPos, i32 ScreenWidth, i32 ScreenHeight);

void
DEBUG_RenderStringVAO(u32 VAO, i32 TextCount, i32 FontID);

#endif
