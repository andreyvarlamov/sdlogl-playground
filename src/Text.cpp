#include "Text.h"

#include <SDL2/SDL_ttf.h>

void
LoadFont(const char *Path)
{
    TTF_Font *Font = NULL;
    i32 FontSizePoints = 20;
    Font = TTF_OpenFont(Path, FontSizePoints);
    SDL_Color TextColor = { 0, 0, 0 };
    TTF_RenderText_Solid(Font, "test", TextColor);
}