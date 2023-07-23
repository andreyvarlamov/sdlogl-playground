#ifndef TEXT_H
#define TEXT_H

#include <glm/glm.hpp>

#include "Common.h"

struct glyph_info
{
    i32 AtlasPosX;
    i32 AtlasPosY;

    i32 MinX;
    i32 MaxX;
    i32 MinY;
    i32 MaxY;

    i32 Advance;
};

struct font_info
{
    u32 AtlasGLID;

    i32 AtlasWidth;
    i32 AtlasHeight;

    i32 Points;
    i32 Height;

    glyph_info GlyphInfos[128];
};

struct ui_string
{
    i32 XPos;
    i32 YPos;
    i32 ScreenWidth;
    i32 ScreenHeight;
    font_info *FontInfo;
    i32 StringLength;

    u32 VAO;
    u32 VBO;

    size_t PositionsBufferSize;
    f32 *Positions;
    size_t UVsBufferSize;
    f32 *UVs;
};

font_info *
RasterizeAndProcessFont(const char *FontPath, i32 FontSizePoints);

ui_string
PrepareUIString(const char *Text, font_info *FontInfo,
                i32 XPos, i32 YPos, i32 ScreenWidth, i32 ScreenHeight);

void
RenderUIString(ui_string UIString);

void
UpdateUIString(ui_string UIString, const char *NewText);

//void
//FreeFontInfoAndUnloadFromGPU(font_info *FontInfo);

void
UnloadUIStringFromGPU(ui_string UIString);

void
CalculateUIStringOffsetPosition(i32 XPos, i32 YPos, const char *OffsetColsByString, i32 OffsetRows,
                                font_info *FontInfo, i32 *Out_OffsetXPos, i32 *Out_OffsetYPos);

#endif
