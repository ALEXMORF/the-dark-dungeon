#pragma once

#include <stdint.h>
#include <math.h>
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int bool32;
typedef bool bool8;
typedef float real32;
typedef double real64;

#define global_variable static
#define internal static
#define local_persist static

#define kilobytes(Value) ((Value)*1024LL)
#define megabytes(Value) (kilobytes(Value)*1024LL)
#define gigabytes(Value) (megabytes(Value)*1024LL)
#define terabytes(Value) (gigabytes(Value)*1024LL)

#define assert(value) do {if(!(value)) *(int*)0 = 0;} while (0)

#define PLATFORM_LOAD_IMAGE(name) uint8 * name(const char *filename, int32 *width, int32 *height, int32 *bytes_per_pixel, int32 default_byte_per_pixel)
typedef PLATFORM_LOAD_IMAGE(Platform_Load_Image);

#define PLATFORM_FREE_IMAGE(name) void name(void *data)
typedef PLATFORM_FREE_IMAGE(Platform_Free_Image);

#define PLATFORM_ALLOCATE_MEMORY(name) void *name(size_t size)
typedef PLATFORM_ALLOCATE_MEMORY(Platform_Allocate_Memory);

#define PLATFORM_PLAY_SOUND(name) void name(char *filename)
typedef PLATFORM_PLAY_SOUND(Platform_Play_Sound);

struct Game_Memory
{
    void *permanent_storage;
    uint32 permanent_storage_size;
    void *transient_storage;
    uint32 transient_storage_size;

    bool32 is_initialized;

    Platform_Load_Image *platform_load_image;
    Platform_Free_Image *platform_free_image;
    Platform_Allocate_Memory *platform_allocate_memory;
};

struct Game_Offscreen_Buffer
{
    int width;
    int height;
    int pitch;
    void *memory;
};

struct Game_Sound_Buffer
{
    void *memory;
    uint32 running_sample_index;
    uint16 sample_count;
};

struct Game_Input
{
    struct
    {
        bool32 left;
        bool32 right;
        bool32 up;
        bool32 down;
        bool32 space;
    } keyboard;
    struct
    {
        real32 dx;
        real32 dy;
        bool32 down;
    } mouse;
    real32 dt_per_frame;
};

#define GAME_UPDATE_AND_RENDER(name) void name(Game_Memory *memory, Game_Input *input, Game_Offscreen_Buffer *buffer)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render);
#define GAME_PROCESS_SOUND(name) void name(Game_Memory *memory, Game_Sound_Buffer *buffer)
typedef GAME_PROCESS_SOUND(Game_Process_Sound);
