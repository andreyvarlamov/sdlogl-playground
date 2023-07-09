#include "Text.h"

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <cstdio>

u32
DEBUG_RenderTextIntoTexture(const char *FontPath, const char *Text)
{
    u32 Result;

    TTF_Font *Font = NULL;
    i32 FontSizePoints = 20;
    Font = TTF_OpenFont(FontPath, FontSizePoints);
    SDL_Color TextColor = { 255, 255, 255, 255 };
    SDL_Color BgColor = { 0, 0, 0, 255 };
    SDL_Surface *RenderedText = TTF_RenderText_Blended(Font, Text, TextColor);
    u8 *SourcePixels = (u8 *) RenderedText->pixels;
    u8 *DestPixels = (u8 *) RenderedText->pixels;

    Assert(RenderedText);
    Assert(RenderedText->format->BytesPerPixel == 4);
    size_t CurrentPitch = RenderedText->pitch;
    size_t DesiredPitch = RenderedText->w * RenderedText->format->BytesPerPixel;
    for (i32 Row = 0; Row < RenderedText->h; ++Row)
    {
        SDL_memmove(DestPixels, SourcePixels, DesiredPitch);
        DestPixels += DesiredPitch;
        SourcePixels += CurrentPitch;
    }

    //SDL_SaveBMP(RenderedText, "test.bmp");

    //fprintf(stderr, "%s\n", SDL_GetError());

    glGenTextures(1, &Result);
    glBindTexture(GL_TEXTURE_2D, Result);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 RenderedText->w, RenderedText->h,
                 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                 RenderedText->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // TODO: Deallocate surfaces, etc.

    return Result;
}