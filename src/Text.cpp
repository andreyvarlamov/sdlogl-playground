#include "Text.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <cstdio>
#include <cstdlib>

u32
DEBUG_RenderTextIntoTexture(const char *FontPath, const char *Text, glm::mat4 *Out_NativeScaleTransform)
{
    u32 Result;

    TTF_Font *Font = NULL;
    i32 FontSizePoints = 20;
    Font = TTF_OpenFont(FontPath, FontSizePoints);
    SDL_Color TextColor = { 255, 255, 255, 255 };
    SDL_Color BgColor = { 0, 0, 0, 255 };
    SDL_Surface *RenderedText = TTF_RenderText_Blended(Font, Text, TextColor);
    Assert(RenderedText);
    Assert(RenderedText->format->BytesPerPixel == 4);

    i32 Width = RenderedText->w;
    i32 Height = RenderedText->h;
    i32 SourcePitch = RenderedText->pitch;
    i32 DestPitch = Width * RenderedText->format->BytesPerPixel;
    u8 *ProcessedPixels = (u8 *) malloc(Height * DestPitch);
    u8 *SourcePixels = (u8 *) RenderedText->pixels;
    u8 *DestPixels = ProcessedPixels + (Height - 1) * DestPitch;
    for (i32 Row = 0; Row < Height; ++Row)
    {
        Assert(DestPixels > 0);
        Assert((SourcePixels - RenderedText->pixels) < Height*SourcePitch);
        for (i32 Column = 0; Column < DestPitch; ++Column)
        {
            *(DestPixels + Column) = *(SourcePixels + Column);
        }
        SourcePixels += SourcePitch;
        DestPixels -= DestPitch;
    }

    //SDL_SaveBMP(RenderedText, "test.bmp");

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

    SDL_FreeSurface(RenderedText);
    free(ProcessedPixels);

    f32 TextScaleWidth = (f32) Width / 1280.0f;
    f32 TextScaleHeight = (f32) Height / 720.0f;
    *Out_NativeScaleTransform = glm::scale(glm::mat4(1.0f), glm::vec3(TextScaleWidth, TextScaleHeight, 1.0f));

    return Result;
}