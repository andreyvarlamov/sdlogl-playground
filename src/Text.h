#ifndef TEXT_H
#define TEXT_H

#include <glm/glm.hpp>

#include "Common.h"

u32
DEBUG_RenderTextIntoTexture(const char *FontPath, const char *Text, glm::mat4 *Out_NativeScaleTransform);

#endif
