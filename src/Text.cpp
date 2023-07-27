#include "Text.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <cstdio>
#include <cstdlib>

#include "Util.h"

void
PrepareRenderDataForString(const char *String, i32 StringLength, i32 OldStringLength, font_info *FontInfo, 
                           i32 XPos, i32 YPos, i32 ScreenWidth, i32 ScreenHeight,
                           f32 *Out_Positions, f32 *Out_UVs);

font_info *
RasterizeAndProcessFont(const char *FontPath, i32 FontSizePoints)
{
    font_info *Result = (font_info *) malloc(sizeof(font_info));

    TTF_Font *Font = NULL;
    Font = TTF_OpenFont(FontPath, FontSizePoints);
    Result->Height = TTF_FontHeight(Font);
    Result->Points = FontSizePoints;

    glyph_info *GlyphInfos = Result->GlyphInfos;
    SDL_Surface *RenderedGlyphs[128];
    SDL_Color WhiteColor = { 255, 255, 255,255 };
    i32 AtlasCellWidth = 0;
    i32 AtlasCellHeight = Result->Height;
    for (u8 Glyph = 32; Glyph < 128; ++Glyph)
    {
        i32 *MinX = &(GlyphInfos[Glyph].MinX);
        i32 *MaxX = &(GlyphInfos[Glyph].MaxX);
        i32 *MinY = &(GlyphInfos[Glyph].MinY);
        i32 *MaxY = &(GlyphInfos[Glyph].MaxY);
        i32 *Advance = &(GlyphInfos[Glyph].Advance);
        TTF_GlyphMetrics(Font, Glyph, MinX, MaxX, MinY, MaxY, Advance);

        SDL_Surface *RenderedGlyph = TTF_RenderGlyph_Blended(Font, Glyph, WhiteColor);
        if (RenderedGlyph->w > AtlasCellWidth)
        {
            AtlasCellWidth = RenderedGlyph->w;
        }
        Assert(RenderedGlyph->h <= AtlasCellHeight);
        
        RenderedGlyphs[Glyph] = RenderedGlyph;
    }

    // 12x8 = 96 -> for 95 (visible) glyphs
    i32 AtlasColumns = 12;
    i32 AtlasRows = 8;
    i32 AtlasWidth = AtlasColumns * AtlasCellWidth;
    i32 AtlasHeight = AtlasRows * AtlasCellHeight;
    Result->AtlasWidth = AtlasWidth;
    Result->AtlasHeight = AtlasHeight;
    SDL_Surface *FontAtlas = SDL_CreateRGBSurface(0,
                                                  AtlasWidth,
                                                  AtlasHeight,
                                                  32,
                                                  0x00FF0000,
                                                  0x0000FF00,
                                                  0x000000FF,
                                                  0xFF000000);
    i32 BytesPerPixel = 4;
    Assert(FontAtlas->format->BytesPerPixel == BytesPerPixel);
    Assert((FontAtlas->pitch) == (FontAtlas->w * FontAtlas->format->BytesPerPixel));

    i32 CurrentAtlasIndex = 0;
    for (u8 Glyph = 32; Glyph < 128; ++Glyph)
    {
        SDL_Surface *RenderedGlyph = RenderedGlyphs[Glyph];
        glyph_info *GlyphInfo = &Result->GlyphInfos[Glyph];

        Assert(RenderedGlyph);
        Assert(RenderedGlyph->format->BytesPerPixel == BytesPerPixel);

        i32 GlyphWidth = RenderedGlyph->w;
        i32 GlyphHeight = RenderedGlyph->h;
        Assert(GlyphWidth <= AtlasCellWidth);
        Assert(GlyphHeight <= AtlasCellHeight);

        i32 CurrentAtlasRow = CurrentAtlasIndex / AtlasColumns;
        i32 CurrentAtlasCol = CurrentAtlasIndex % AtlasColumns;
        GlyphInfo->AtlasPosX = CurrentAtlasCol * AtlasCellWidth;
        GlyphInfo->AtlasPosY = CurrentAtlasRow * AtlasCellHeight;
        i32 AtlasPosByteOffset = (GlyphInfo->AtlasPosY * AtlasWidth + GlyphInfo->AtlasPosX) * BytesPerPixel;

        u8 *DestBytes = ((u8 *) FontAtlas->pixels) + AtlasPosByteOffset;
        u8 *SourceBytes = ((u8 *) RenderedGlyph->pixels);
        for (i32 GlyphRow = 0; GlyphRow < GlyphHeight; ++GlyphRow)
        {
            for (i32 GlyphCol = 0; GlyphCol < GlyphWidth; ++GlyphCol)
            {
                u8 *DestPixelByte = DestBytes + GlyphCol * BytesPerPixel;
                u8 *SourcePixelByte = SourceBytes + GlyphCol * BytesPerPixel;
                for (i32 ByteIndex = 0; ByteIndex < BytesPerPixel; ++ByteIndex)
                {
                    *DestPixelByte++ = *SourcePixelByte++;
                }
            }
            SourceBytes += RenderedGlyph->pitch;
            DestBytes += FontAtlas->pitch;
        }
        ++CurrentAtlasIndex;

        //if (Glyph == 'p')
        //{
        //    SDL_SaveBMP(RenderedGlyphs[Glyph], "letter_test.bmp");
        //}

        SDL_FreeSurface(RenderedGlyphs[Glyph]);
        RenderedGlyphs[Glyph] = 0;
    }

    glGenTextures(1, &Result->AtlasGLID);
    glBindTexture(GL_TEXTURE_2D, Result->AtlasGLID);
    // TODO: Don't really need all 4 channels, just need alpha, and can color in the shader with custom color
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 AtlasWidth, AtlasHeight,
                 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                 FontAtlas->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    TTF_CloseFont(Font);
    //SDL_SaveBMP(FontAtlas, "font_atlas_test.bmp");
    SDL_FreeSurface(FontAtlas);

    return Result;
}

ui_string
PrepareUIString(const char *Text, font_info *FontInfo,
                i32 XPos, i32 YPos, i32 ScreenWidth, i32 ScreenHeight)
{
    ui_string Result;

    Result.XPos = XPos;
    Result.YPos = YPos;
    Result.ScreenWidth = ScreenWidth;
    Result.ScreenHeight = ScreenHeight;
    Result.FontInfo = FontInfo;



    Result.StringLength = GetNullTerminatedStringLength(Text);

    Result.PositionsBufferSize = Result.StringLength * 12 * sizeof(f32);
    Result.Positions = (f32 *) malloc(Result.PositionsBufferSize);
    Result.UVsBufferSize = Result.StringLength * 12 * sizeof(f32);
    Result.UVs = (f32 *) malloc(Result.UVsBufferSize);

    PrepareRenderDataForString(Text, Result.StringLength, Result.StringLength, FontInfo,
                               Result.XPos, Result.YPos, Result.ScreenWidth, Result.ScreenHeight,
                               Result.Positions, Result.UVs);

    glGenVertexArrays(1, &Result.VAO);
    glGenBuffers(1, &Result.VBO);
    glBindVertexArray(Result.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, Result.VBO);
    glBufferData(GL_ARRAY_BUFFER, Result.PositionsBufferSize + Result.UVsBufferSize, 0, GL_STATIC_DRAW); // TODO: Dynamic?
    glBufferSubData(GL_ARRAY_BUFFER, 0, Result.PositionsBufferSize, Result.Positions);
    glBufferSubData(GL_ARRAY_BUFFER, Result.PositionsBufferSize, Result.UVsBufferSize, Result.UVs);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void *) 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void *) Result.PositionsBufferSize);

    glBindVertexArray(0);

    return Result;
}

void
RenderUIString(ui_string UIString)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, UIString.FontInfo->AtlasGLID);

    glBindVertexArray(UIString.VAO);
    glDrawArrays(GL_TRIANGLES, 0, UIString.StringLength * 6);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void
UpdateUIString(ui_string UIString, const char *NewText)
{
    i32 NewTextLength = GetNullTerminatedStringLength(NewText);
    Assert(NewTextLength <= UIString.StringLength);

    // NOTE: Do not want to update text length in UIString, because the buffer is capable of holding
    //       the same length of strings throughout its lifetime.
    PrepareRenderDataForString(NewText, NewTextLength, UIString.StringLength, UIString.FontInfo,
                               UIString.XPos, UIString.YPos, UIString.ScreenWidth, UIString.ScreenHeight,
                               UIString.Positions, UIString.UVs);

    glBindBuffer(GL_ARRAY_BUFFER, UIString.VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, UIString.PositionsBufferSize, UIString.Positions);
    glBufferSubData(GL_ARRAY_BUFFER, UIString.PositionsBufferSize, UIString.UVsBufferSize, UIString.UVs);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
PrepareRenderDataForString(const char *String, i32 StringLength, i32 BufferLength, font_info *FontInfo, 
                        i32 XPos, i32 YPos, i32 ScreenWidth, i32 ScreenHeight,
                        f32 *Out_Positions, f32 *Out_UVs)
{
    Assert(StringLength <= BufferLength);

    f32 HalfScreenWidth = (f32) ScreenWidth / 2.0f;
    f32 HalfScreenHeight = (f32) ScreenHeight / 2.0f;
    f32 AtlasWidth = (f32) FontInfo->AtlasWidth;
    f32 AtlasHeight = (f32) FontInfo->AtlasHeight;

    i32 CurrentX = XPos;
    i32 CurrentY = YPos;
    for (i32 TextIndex = 0; TextIndex < BufferLength; ++TextIndex)
    {
        u8 Glyph;
        if (TextIndex < StringLength)
        {
            Glyph = String[TextIndex];
        }
        else
        {
            Glyph = ' ';
        }

        if (Glyph == '\n')
        {
            CurrentX = XPos;
            CurrentY += FontInfo->Height;
            continue;
        }

        glyph_info *GlyphInfo = &FontInfo->GlyphInfos[Glyph];

        // NOTE: It seems that SDL_ttf embeds MinX into the rendered glyph, but also it's ignored if it's less than 0
        //       Need to shift where to place glyph if MinX is negative, but if not negative, it's already included
        //       in the rendered glyph. This works but seems very finicky
        // TODO: Just use freetype directly or stb (or direct GPU render from ttf????????)
        i32 OnScreenX = ((GlyphInfo->MinX >= 0) ? (CurrentX) : (CurrentX + GlyphInfo->MinX));
        i32 OnScreenY = CurrentY;
        i32 OnScreenWidth = ((GlyphInfo->MinX >= 0) ? (GlyphInfo->MaxX) : (GlyphInfo->MaxX - GlyphInfo->MinX));
        i32 OnScreenHeight = FontInfo->Height;
        f32 NDCLeft = ((f32) OnScreenX / HalfScreenWidth) - 1.0f;
        f32 NDCTop = -(((f32) OnScreenY / HalfScreenHeight) - 1.0f);
        f32 NDCRight = ((f32) (OnScreenX + OnScreenWidth) / HalfScreenWidth) - 1.0f;
        f32 NDCBottom = -(((f32) (OnScreenY + OnScreenHeight) / HalfScreenHeight) - 1.0f);

        i32 TextureX = GlyphInfo->AtlasPosX;
        i32 TextureY = GlyphInfo->AtlasPosY;
        i32 TextureWidth = ((GlyphInfo->MinX >= 0) ? (GlyphInfo->MaxX) : (GlyphInfo->MaxX - GlyphInfo->MinX));
        i32 TextureHeight = FontInfo->Height;
        f32 UVLeft = (f32) TextureX / AtlasWidth;
        f32 UVTop = (f32) TextureY / AtlasHeight;
        f32 UVRight = (f32) (TextureX + TextureWidth) / AtlasWidth;
        f32 UVBottom = (f32) (TextureY + TextureHeight) / AtlasHeight;

        Out_Positions[TextIndex * 12 + 0]  = NDCLeft;  Out_Positions[TextIndex * 12 + 1]  = NDCTop;
        Out_Positions[TextIndex * 12 + 2]  = NDCLeft;  Out_Positions[TextIndex * 12 + 3]  = NDCBottom;
        Out_Positions[TextIndex * 12 + 4]  = NDCRight; Out_Positions[TextIndex * 12 + 5]  = NDCTop;
        Out_Positions[TextIndex * 12 + 6]  = NDCRight; Out_Positions[TextIndex * 12 + 7]  = NDCTop;
        Out_Positions[TextIndex * 12 + 8]  = NDCLeft;  Out_Positions[TextIndex * 12 + 9]  = NDCBottom;
        Out_Positions[TextIndex * 12 + 10] = NDCRight; Out_Positions[TextIndex * 12 + 11] = NDCBottom;

        Out_UVs[TextIndex * 12 + 0]  = UVLeft;  Out_UVs[TextIndex * 12 + 1]  = UVTop;
        Out_UVs[TextIndex * 12 + 2]  = UVLeft;  Out_UVs[TextIndex * 12 + 3]  = UVBottom;
        Out_UVs[TextIndex * 12 + 4]  = UVRight; Out_UVs[TextIndex * 12 + 5]  = UVTop;
        Out_UVs[TextIndex * 12 + 6]  = UVRight; Out_UVs[TextIndex * 12 + 7]  = UVTop;
        Out_UVs[TextIndex * 12 + 8]  = UVLeft;  Out_UVs[TextIndex * 12 + 9]  = UVBottom;
        Out_UVs[TextIndex * 12 + 10] = UVRight; Out_UVs[TextIndex * 12 + 11] = UVBottom;

        CurrentX += GlyphInfo->Advance;
    }
}

void
CalculateUIStringOffsetPosition(i32 XPos, i32 YPos, const char *OffsetColsByString, i32 OffsetRows,
                                font_info *FontInfo, i32 *Out_OffsetXPos, i32 *Out_OffsetYPos)
{
    if (Out_OffsetXPos)
    {
        *Out_OffsetXPos = XPos;

        if (OffsetColsByString)
        {
            while(*OffsetColsByString)
            {
                *Out_OffsetXPos += FontInfo->GlyphInfos[*OffsetColsByString].Advance;
                OffsetColsByString++;
            }
        }
    }

    if (Out_OffsetYPos)
    {
        *Out_OffsetYPos = YPos;
        *Out_OffsetYPos += OffsetRows * FontInfo->Height;
    }
}