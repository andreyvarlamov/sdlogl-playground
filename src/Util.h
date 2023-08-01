#ifndef UTIL_H
#define UTIL_H

#include "Common.h"

u32
LoadTexture(const char *Path, bool GenerateMipmap);
char *
ReadFile(const char *Path, size_t *Out_Size);

void
PrintVBODataF(u32 VBO, size_t FloatCount);

i32
GetNullTerminatedStringLength(const char *String);
void 
CatStrings(char *SourceA, i32 SourceACount,
           char *SourceB, i32 SourceBCount,
           char *Out_Dest, i32 DestBufferSize);
void
GetFileDirectory(char *FilePath, i32 FilePathCount,
                 char *Out_FileDirectory, i32 *Out_FileDirectoryCount,
                 i32 FileDirectoryBufferSize);



#endif
