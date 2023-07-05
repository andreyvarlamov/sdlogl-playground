#ifndef UTIL_H
#define UTIL_H

#include "Common.h"
#include <string>

u32 LoadTexture(const char* Path);
std::string readFile(const char *path);
u32 buildShader(const char *vertexPath, const char *fragmentPath);

i32 GetNullTerminatedStringLength(char *String);
void CatStrings(char *SourceA, i32 SourceACount,
                char *SourceB, i32 SourceBCount,
                char *Out_Dest, i32 DestBufferSize);
void GetFileDirectory(char *FilePath, i32 FilePathCount,
                      char *Out_FileDirectory, i32 *Out_FileDirectoryCount,
                      i32 FileDirectoryBufferSize);

#endif
