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

u32
DEBUG_RenderTextIntoTexture(const char *FontPath, const char *Text, glm::mat4 *Out_NativeScaleTransform)
{
    u32 Result;

    TTF_Font *Font = NULL;
    i32 FontSizePoints = 20;
    Font = TTF_OpenFont(FontPath, FontSizePoints);

    i32 FontHeight = TTF_FontHeight(Font);

    glyph_info GlyphInfos[128];
    SDL_Surface *RenderedGlyphs[128];

    SDL_Color WhiteColor = { 255, 0,0,255 };

    i32 MaxGlyphWidth = 0;
    i32 MaxGlyphHeight = FontHeight;
    for (u8 Glyph = 32; Glyph < 33; ++Glyph)
    {
        i32 *MinX = &(GlyphInfos[Glyph].MinX);
        i32 *MaxX = &(GlyphInfos[Glyph].MaxX);
        i32 *MinY = &(GlyphInfos[Glyph].MinY);
        i32 *MaxY = &(GlyphInfos[Glyph].MaxY);
        i32 *Advance = &(GlyphInfos[Glyph].Advance);
        TTF_GlyphMetrics(Font, Glyph, MinX, MaxX, MinY, MaxY, Advance);

        SDL_Surface *RenderedGlyph = TTF_RenderGlyph_Blended(Font, 'A', WhiteColor);
        SDL_SaveBMP(RenderedGlyph, "g.bmp");
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
    i32 AtlasWidth = 12 * MaxGlyphWidth;
    i32 AtlasHeight = 8 * MaxGlyphHeight;
    SDL_Surface *FontAtlas = SDL_CreateRGBSurface(0,
                                                  AtlasWidth,
                                                  AtlasHeight,
                                                  32,
                                                  0x000000FF,
                                                  0x0000FF00,
                                                  0x00FF0000,
                                                  0xFF000000);
    i32 BytesPerPixel = 4;
    Assert(FontAtlas->format->BytesPerPixel == BytesPerPixel);
    Assert((FontAtlas->pitch) == (FontAtlas->w * FontAtlas->format->BytesPerPixel));

    i32 CurrentAtlasIndex = 0;
    for (u8 Glyph = 32; Glyph < 33; ++Glyph)
    {
        SDL_Surface *RenderedGlyph = RenderedGlyphs[Glyph];
        Assert(RenderedGlyph);
        Assert(RenderedGlyph->format->BytesPerPixel == BytesPerPixel);

        i32 GlyphWidth = RenderedGlyph->w;
        i32 GlyphHeight = RenderedGlyph->h;
        //printf("%c: W:%d; H:%d;\tCalc: X:Mx:%d,Mn:%d,W:%d;\tY:Mx:%d,Mn:%d,H:%d\n",
        //       Glyph, GlyphWidth, GlyphHeight,
        //       GlyphInfos[Glyph].MaxX, GlyphInfos[Glyph].MinX, GlyphInfos[Glyph].MaxX - GlyphInfos[Glyph].MinX,
        //       GlyphInfos[Glyph].MaxY, GlyphInfos[Glyph].MinY, GlyphInfos[Glyph].MaxY - GlyphInfos[Glyph].MinY);
        Assert(GlyphWidth <= MaxGlyphWidth);
        Assert(GlyphHeight <= MaxGlyphHeight);

        i32 CurrentAtlasRow = CurrentAtlasIndex / AtlasWidth;
        i32 CurrentAtlasCol = CurrentAtlasIndex % AtlasWidth;
        i32 GlyphRowOffset = CurrentAtlasRow * MaxGlyphHeight;
        i32 GlyphColOffset = CurrentAtlasCol * MaxGlyphWidth;
        i32 GlyphOffset = GlyphRowOffset + GlyphColOffset;
        i32 ByteOffsetToPositionInAtlas = GlyphOffset * BytesPerPixel;

        u8 *DestBytes = ((u8 *) FontAtlas->pixels) + ByteOffsetToPositionInAtlas;
        u8 *SourceBytes = ((u8 *) RenderedGlyph->pixels) + RenderedGlyph->pitch * GlyphHeight;
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
            SourceBytes -= RenderedGlyph->pitch;
            DestBytes += FontAtlas->pitch;
        }

        SDL_FreeSurface(RenderedGlyphs[Glyph]);
        RenderedGlyphs[Glyph] = 0;
    }
    SDL_SaveBMP(FontAtlas, "test.bmp");

    SDL_Color BgColor = { 0, 0, 0, 255 };
    SDL_Surface *RenderedText = TTF_RenderText_Blended(Font, Text, WhiteColor);
    Assert(RenderedText);
    Assert(RenderedText->format->BytesPerPixel == 4);

    i32 Width = RenderedText->w;
    i32 Height = RenderedText->h;
    i32 SourcePitch = RenderedText->pitch;
    i32 DestPitch = Width * RenderedText->format->BytesPerPixel;
    u8 *ProcessedPixels = (u8 *) malloc(Height * DestPitch);


    //fprintf(stderr, "%s\n", SDL_GetError());

    glGenTextures(1, &Result);
    glBindTexture(GL_TEXTURE_2D, Result);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 Width, Height,
                 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                 ProcessedPixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    TTF_CloseFont(Font);
    SDL_FreeSurface(RenderedText);
    free(ProcessedPixels);

    f32 TextScaleWidth = (f32) Width / 1280.0f;
    f32 TextScaleHeight = (f32) Height / 720.0f;
    *Out_NativeScaleTransform = glm::scale(glm::mat4(1.0f), glm::vec3(TextScaleWidth, TextScaleHeight, 1.0f));

    return Result;
}