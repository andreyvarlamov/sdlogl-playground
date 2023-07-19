#include "Text.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <cstdio>
#include <cstdlib>

struct glyph_info
{
    i32 MinX;
    i32 MaxX;
    i32 MinY;
    i32 MaxY;
    i32 Advance;
};

#define FONT_COUNT 4
#define GLYPH_COUNT 128

struct font_info
{
    u32 AtlasGLID;
    glyph_info GlyphInfos[GLYPH_COUNT];
};

font_info FontInfos[FONT_COUNT];
i32 NextUnusedFontID = 0;

i32
DEBUG_RasterizeFontIntoGLTexture(const char *FontPath, i32 FontSizePoints)
{
    Assert(NextUnusedFontID < FONT_COUNT);
    i32 FontID = NextUnusedFontID++;

    TTF_Font *Font = NULL;
    Font = TTF_OpenFont(FontPath, FontSizePoints);

    i32 FontHeight = TTF_FontHeight(Font);

    glyph_info *GlyphInfos = FontInfos[FontID].GlyphInfos;
    SDL_Surface *RenderedGlyphs[128];
    SDL_Color WhiteColor = { 255, 255, 255,255 };
    i32 MaxGlyphWidth = 0;
    i32 MaxGlyphHeight = FontHeight;
    for (u8 Glyph = 32; Glyph < 128; ++Glyph)
    {
        i32 *MinX = &(GlyphInfos[Glyph].MinX);
        i32 *MaxX = &(GlyphInfos[Glyph].MaxX);
        i32 *MinY = &(GlyphInfos[Glyph].MinY);
        i32 *MaxY = &(GlyphInfos[Glyph].MaxY);
        i32 *Advance = &(GlyphInfos[Glyph].Advance);
        TTF_GlyphMetrics(Font, Glyph, MinX, MaxX, MinY, MaxY, Advance);

        SDL_Surface *RenderedGlyph = TTF_RenderGlyph_Blended(Font, Glyph, WhiteColor);
        if (RenderedGlyph->w > MaxGlyphWidth)
        {
            MaxGlyphWidth = RenderedGlyph->w;
        }
        if (RenderedGlyph->h > MaxGlyphHeight)
        {
            MaxGlyphHeight = RenderedGlyph->h;
        }
        
        RenderedGlyphs[Glyph] = RenderedGlyph;
    }

    // 12x8 = 96 -> for 95 (visible) glyphs
    i32 AtlasColumns = 12;
    i32 AtlasRows = 8;
    i32 AtlasWidth = AtlasColumns * MaxGlyphWidth;
    i32 AtlasHeight = AtlasRows * MaxGlyphHeight;
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
        Assert(RenderedGlyph);
        Assert(RenderedGlyph->format->BytesPerPixel == BytesPerPixel);

        i32 GlyphWidth = RenderedGlyph->w;
        i32 GlyphHeight = RenderedGlyph->h;
        Assert(GlyphWidth <= MaxGlyphWidth);
        Assert(GlyphHeight <= MaxGlyphHeight);

        i32 CurrentAtlasRow = CurrentAtlasIndex / AtlasColumns;
        i32 CurrentAtlasCol = CurrentAtlasIndex % AtlasColumns;
        i32 GlyphRowOffset = CurrentAtlasRow * AtlasWidth * MaxGlyphHeight;
        i32 GlyphColOffset = CurrentAtlasCol * MaxGlyphWidth;
        i32 GlyphOffset = GlyphRowOffset + GlyphColOffset;
        i32 ByteOffsetToPositionInAtlas = GlyphOffset * BytesPerPixel;

        u8 *DestBytes = ((u8 *) FontAtlas->pixels) + ByteOffsetToPositionInAtlas;
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

        SDL_FreeSurface(RenderedGlyphs[Glyph]);
        RenderedGlyphs[Glyph] = 0;
    }


    glGenTextures(1, &FontInfos[FontID].AtlasGLID);
    glBindTexture(GL_TEXTURE_2D, FontInfos[FontID].AtlasGLID);
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
    SDL_FreeSurface(FontAtlas);

    return FontID;
}

u32
DEBUG_PrepareRenderDataForString(i32 FontID, const char *Text, i32 TextCount,
                                 i32 XPos, i32 YPos, i32 ScreenWidth, i32 ScreenHeight)
{
    u32 VAO;

    font_info *FontInfo = &FontInfos[FontID];

    i32 CurrentX = XPos; // NDC?
    i32 CurrentY = YPos;

    for (i32 TextIndex = 0; TextIndex < TextCount; ++TextCount)
    {
        u8 Glyph = Text[TextCount];

        glyph_info *GlyphInfo = &FontInfo->GlyphInfos[Glyph];

        // Atlas px position
        // Atlas Cell Width, Cell Height
        // Glyph MinX, MaxX, MinY, MaxY, Advance
    }
}