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
    i32 AtlasPosX;
    i32 AtlasPosY;

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
    i32 AtlasWidth;
    i32 AtlasHeight;

    i32 Points;
    i32 Height;
    i32 Ascent;

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
    FontInfos[FontID].Ascent = TTF_FontAscent(Font);
    FontInfos[FontID].Height = TTF_FontHeight(Font);

    glyph_info *GlyphInfos = FontInfos[FontID].GlyphInfos;
    SDL_Surface *RenderedGlyphs[128];
    SDL_Color WhiteColor = { 255, 255, 255,255 };
    i32 AtlasCellWidth = 0;
    i32 AtlasCellHeight = FontInfos[FontID].Height;
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
    FontInfos[FontID].AtlasWidth = AtlasWidth;
    FontInfos[FontID].AtlasHeight = AtlasHeight;
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
        glyph_info *GlyphInfo = &FontInfos[FontID].GlyphInfos[Glyph];

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
    //SDL_SaveBMP(FontAtlas, "font_atlas_test.bmp");
    SDL_FreeSurface(FontAtlas);

    return FontID;
}

#if 1
u32
DEBUG_PrepareRenderDataForString(i32 FontID, const char *Text, i32 TextCount,
                                 i32 XPos, i32 YPos, i32 ScreenWidth, i32 ScreenHeight)
{
    font_info *FontInfo = &FontInfos[FontID];

    i32 CurrentX = XPos;
    i32 CurrentY = YPos;

    size_t PositionsBufferSize = TextCount * 12 * sizeof(f32);
    f32 *Positions = (f32 *)malloc(PositionsBufferSize);
    size_t UVsBufferSize = TextCount * 12 * sizeof(f32);
    f32 *UVs = (f32 *)malloc(UVsBufferSize);

    f32 HalfScreenWidth = (f32) ScreenWidth / 2.0f;
    f32 HalfScreenHeight = (f32) ScreenHeight / 2.0f;
    f32 AtlasWidth = (f32) FontInfo->AtlasWidth;
    f32 AtlasHeight = (f32) FontInfo->AtlasHeight;

    for (i32 TextIndex = 0; TextIndex < TextCount; ++TextIndex)
    {
        u8 Glyph = Text[TextIndex];

        if (Glyph == '\n')
        {
            CurrentX = XPos;
            CurrentY += FontInfo->Height;
            continue;
        }

        glyph_info *GlyphInfo = &FontInfo->GlyphInfos[Glyph];

        i32 OnScreenX = CurrentX + GlyphInfo->MinX;
        i32 OnScreenY = CurrentY + FontInfo->Ascent - GlyphInfo->MaxY;
        i32 OnScreenWidth = GlyphInfo->MaxX - GlyphInfo->MinX; OnScreenWidth *= 10.0f;
        i32 OnScreenHeight = GlyphInfo->MaxY - GlyphInfo->MinY; OnScreenHeight *= 10.0f;
        f32 NDCLeft = ((f32) OnScreenX / HalfScreenWidth) - 1.0f;
        f32 NDCTop = -(((f32) OnScreenY / HalfScreenHeight) - 1.0f);
        f32 NDCRight = ((f32) (OnScreenX + OnScreenWidth) / HalfScreenWidth) - 1.0f;
        f32 NDCBottom = -(((f32) (OnScreenY + OnScreenHeight) / HalfScreenHeight) - 1.0f);
        //NDCLeft = -1.0f;
        //NDCTop = 1.0f;
        //NDCRight = 1.0f;
        //NDCBottom = -1.0f;

        i32 TextureX = GlyphInfo->AtlasPosX;
        i32 TextureY = GlyphInfo->AtlasPosY;
        i32 TextureWidth = GlyphInfo->MaxX;
        i32 TextureHeight = FontInfo->Height;
        f32 UVLeft = (f32) TextureX / AtlasWidth;
        f32 UVTop = (f32) TextureY / AtlasHeight;
        f32 UVRight = (f32) (TextureX + TextureWidth) / AtlasWidth;
        f32 UVBottom = (f32) (TextureY + TextureHeight) / AtlasHeight;
        //UVLeft = 0.0f;
        //UVTop = 1.0f;
        //UVRight = 1.0f;
        //UVBottom = 0.0f;

        Positions[TextIndex * 12 + 0]  = NDCLeft;  Positions[TextIndex * 12 + 1]  = NDCTop;
        Positions[TextIndex * 12 + 2]  = NDCLeft;  Positions[TextIndex * 12 + 3]  = NDCBottom;
        Positions[TextIndex * 12 + 4]  = NDCRight; Positions[TextIndex * 12 + 5]  = NDCTop;
        Positions[TextIndex * 12 + 6]  = NDCRight; Positions[TextIndex * 12 + 7]  = NDCTop;
        Positions[TextIndex * 12 + 8]  = NDCLeft;  Positions[TextIndex * 12 + 9]  = NDCBottom;
        Positions[TextIndex * 12 + 10] = NDCRight; Positions[TextIndex * 12 + 11] = NDCBottom;

        UVs[TextIndex * 12 + 0]  = UVLeft;  UVs[TextIndex * 12 + 1]  = UVTop;
        UVs[TextIndex * 12 + 2]  = UVLeft;  UVs[TextIndex * 12 + 3]  = UVBottom;
        UVs[TextIndex * 12 + 4]  = UVRight; UVs[TextIndex * 12 + 5]  = UVTop;
        UVs[TextIndex * 12 + 6]  = UVRight; UVs[TextIndex * 12 + 7]  = UVTop;
        UVs[TextIndex * 12 + 8]  = UVLeft;  UVs[TextIndex * 12 + 9]  = UVBottom;
        UVs[TextIndex * 12 + 10] = UVRight; UVs[TextIndex * 12 + 11] = UVBottom;

        CurrentX += GlyphInfo->Advance * 10.0f;
    }

    u32 VAO;
    glGenVertexArrays(1, &VAO);
    u32 VBO;
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, PositionsBufferSize + UVsBufferSize, 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, PositionsBufferSize, Positions);
    glBufferSubData(GL_ARRAY_BUFFER, PositionsBufferSize, UVsBufferSize, UVs);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void *) 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void *) PositionsBufferSize);

    glBindVertexArray(0);

    free(Positions);
    free(UVs);

    return VAO;
}

void
DEBUG_RenderStringVAO(u32 VAO, i32 TextCount, i32 FontID)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, FontInfos[FontID].AtlasGLID);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, TextCount * 6);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}
#endif
