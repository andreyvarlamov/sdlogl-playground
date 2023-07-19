#include "Util.h"

#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <cstdlib>
#include <cstdio>

#define LOADED_TEXTURE_CACHE_BLOCK_SIZE 32
struct loaded_textures_cache_block
{
    i32 Count;
    u32 TextureIDs[LOADED_TEXTURE_CACHE_BLOCK_SIZE];
    char TexturePaths[LOADED_TEXTURE_CACHE_BLOCK_SIZE][MAX_PATH_LENGTH];
};

struct loaded_textures_cache
{
    i32 BlockCount;
    loaded_textures_cache_block **Blocks;
};

static loaded_textures_cache LoadedTexturesCache;

static void
InitializeLoadedTexturesCache();
static u32
GetTextureFromCache(char *TexturePath);
static bool
AddLoadedTexturesCacheBlock();
static void
AddTextureToCache(char *TexturePath, u32 TextureID);

// ------------------------
// TEXTURES ---------------
// ------------------------

// TODO: STBI is too slow; switch back to SDL image
u32
LoadTexture(const char *Path, bool GenerateMipmap)
{
    char PathOnStack[MAX_PATH_LENGTH];
    strncpy_s(PathOnStack, Path, MAX_PATH_LENGTH - 1);
    u32 TextureID = GetTextureFromCache(PathOnStack);
    if (TextureID > 0)
    {
        return TextureID;
    }

    int Width, Height, ComponentCount;
    u8 *Data = stbi_load(Path, &Width, &Height, &ComponentCount, 0);
    if (Data)
    {
        GLenum Format;
        if (ComponentCount == 1)
        {
            Format = GL_RED;
        }
        else if (ComponentCount == 3)
        {
            Format = GL_RGB;
        }
        else if (ComponentCount == 4)
        {
            Format = GL_RGBA;
        }
        else
        {
            fprintf(stderr, "Unknown texture format at path: %s\n", Path);
            stbi_image_free(Data);
            return 0;
        }

        glGenTextures(1, &TextureID);
        glBindTexture(GL_TEXTURE_2D, TextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, Format, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Data);
        if (GenerateMipmap)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        if (GenerateMipmap)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        AddTextureToCache(PathOnStack, TextureID);

        stbi_image_free(Data);
        return TextureID;
    }
    else
    {
        fprintf(stderr, "Texture failed to load at path: %s\n", Path);
        return 0;
    }
}

// ------------------------
// FILE IO ----------------
// ------------------------

char *
ReadFile(const char *Path, size_t *Out_Size)
{
    char *Result = 0;

    FILE *File;
    fopen_s(&File, Path, "rb");
    Assert(File);

    fseek(File, 0, SEEK_END);
    size_t Size = ftell(File);
    fseek(File, 0, SEEK_SET);

    Result = (char *) malloc(Size + 1);
    Assert(Result);
    fread(Result, Size, 1, File);
    fclose(File);
    Result[Size] = '\0';

    *Out_Size = Size;
    return Result;
}

// ------------------------
// STRINGS ----------------
// ------------------------

#define MAX_NULL_TERMINATED_SEARCH_LENGTH 1024
i32
GetNullTerminatedStringLength(char *String)
{
    i32 Length = 0;

    while (String[Length] != '\0' &&
           Length < MAX_NULL_TERMINATED_SEARCH_LENGTH)
    {
        ++Length;
    }

    Assert(Length < MAX_NULL_TERMINATED_SEARCH_LENGTH);

    return Length;
}

void
CatStrings(char *SourceA, i32 SourceACount,
           char *SourceB, i32 SourceBCount,
           char *Out_Dest, i32 DestBufferSize)
{
    Assert(SourceACount + SourceBCount < DestBufferSize);

    i32 DestIndex = 0;
    i32 SourceAIndex = 0;
    for (; SourceAIndex < SourceACount; ++SourceAIndex, ++DestIndex)
    {
        Out_Dest[DestIndex] = SourceA[SourceAIndex];
    }
    i32 SourceBIndex = 0;
    for (; SourceBIndex < SourceBCount; ++SourceBIndex, ++DestIndex)
    {
        Out_Dest[DestIndex] = SourceB[SourceBIndex];
    }
    Out_Dest[DestIndex] = '\0';
}

void
GetFileDirectory(char *FilePath, i32 FilePathCount,
                 char *Out_FileDirectory, i32 *Out_FileDirectoryCount,
                 i32 FileDirectoryBufferSize)
{
    i32 LastSlashIndex = FilePathCount - 1;
    for (; LastSlashIndex >= 0; --LastSlashIndex)
    {
        if (FilePath[LastSlashIndex] == '/')
        {
            break;
        }
    }

    Assert(LastSlashIndex < FileDirectoryBufferSize);

    i32 Index = 0;
    for (; Index <= LastSlashIndex; ++Index)
    {
        Out_FileDirectory[Index] = FilePath[Index];
    }

    Out_FileDirectory[Index] = '\0';

    *Out_FileDirectoryCount = Index;
}

// ----------------------------
// INTERNAL HELPERS -----------
// ----------------------------

static void
InitializeLoadedTexturesCache()
{
    if (LoadedTexturesCache.BlockCount == 0)
    {
        LoadedTexturesCache.Blocks = ((loaded_textures_cache_block **)
                                      malloc(sizeof(loaded_textures_cache_block *)));
        if (LoadedTexturesCache.Blocks)
        {
            LoadedTexturesCache.Blocks[0] = ((loaded_textures_cache_block *)
                                             malloc(sizeof(loaded_textures_cache_block)));
            if (LoadedTexturesCache.Blocks[0])
            {
                LoadedTexturesCache.BlockCount = 1;
                LoadedTexturesCache.Blocks[0]->Count = 0;
            }
        }
    }
}

static u32
GetTextureFromCache(char *TexturePath)
{
    if (LoadedTexturesCache.BlockCount == 0)
    {
        InitializeLoadedTexturesCache();
    }

    for (i32 BlockIndex = 0; BlockIndex < LoadedTexturesCache.BlockCount; ++BlockIndex)
    {
        loaded_textures_cache_block *Block = LoadedTexturesCache.Blocks[BlockIndex];

        for (i32 PathIndex = 0; PathIndex < Block->Count; ++PathIndex)
        {
            if (strcmp(Block->TexturePaths[PathIndex], TexturePath) == 0)
            {
                return Block->TextureIDs[PathIndex];
            }
        }
    }

    return 0;
}

static bool
AddLoadedTexturesCacheBlock()
{
    LoadedTexturesCache.Blocks = ((loaded_textures_cache_block **)
                                  realloc(LoadedTexturesCache.Blocks,
                                          (LoadedTexturesCache.BlockCount + 1) *
                                          sizeof(loaded_textures_cache_block *)));

    Assert(LoadedTexturesCache.Blocks);
    if (LoadedTexturesCache.Blocks)
    {
        LoadedTexturesCache.Blocks[LoadedTexturesCache.BlockCount] =
            (loaded_textures_cache_block *) malloc(sizeof(loaded_textures_cache_block));
        Assert(LoadedTexturesCache.Blocks[LoadedTexturesCache.BlockCount]);
        if (LoadedTexturesCache.Blocks[LoadedTexturesCache.BlockCount])
        {
            LoadedTexturesCache.Blocks[LoadedTexturesCache.BlockCount]->Count = 0;
            ++LoadedTexturesCache.BlockCount;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        // TODO: Logging
        return false;
    }
}

static void
AddTextureToCache(char *TexturePath, u32 TextureID)
{
    if (LoadedTexturesCache.BlockCount > 0)
    {
        loaded_textures_cache_block *Block =
            LoadedTexturesCache.Blocks[LoadedTexturesCache.BlockCount - 1];

        if (Block->Count >= LOADED_TEXTURE_CACHE_BLOCK_SIZE)
        {
            if (AddLoadedTexturesCacheBlock())
            {
                Block = LoadedTexturesCache.Blocks[LoadedTexturesCache.BlockCount - 1];
            }
            else
            {
                Block = 0;
            }
        }

        Assert(Block);
        if (Block)
        {
            Block->TextureIDs[Block->Count] = TextureID;
            strncpy_s(Block->TexturePaths[Block->Count], TexturePath, MAX_PATH_LENGTH - 1);
            Block->Count++;
        }
        else
        {
            // TODO: Logging
        }
    }
}
