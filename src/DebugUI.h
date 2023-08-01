#ifndef DEBUGUI_H
#define DEBUGUI_H

#include "Common.h"

void
DEBUG_InitializeDebugUI(i32 X, i32 Y, i32 ScreenWidth, i32 ScreenHeight, u32 Shader);

void
DEBUG_AddDebugString(const char *String);

void
DEBUG_RenderAllDebugStrings();

void
DEBUG_ResetAllDebugStrings();

#endif
