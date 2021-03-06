#pragma once

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <time.h>

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
#define pi32 3.1415926f

#define assert(value) do {if(!(value)) *(int*)0 = 0;} while (0)

#define PLATFORM_LOAD_IMAGE(name) uint8 * name(const char *filename, int32 *width, int32 *height, int32 *bytes_per_pixel, int32 default_byte_per_pixel)
typedef PLATFORM_LOAD_IMAGE(Platform_Load_Image);

#define PLATFORM_FREE_IMAGE(name) void name(void *data)
typedef PLATFORM_FREE_IMAGE(Platform_Free_Image);

#define PLATFORM_LOAD_AUDIO(name) void * name(char *filename, int32 *channels, int32 *byte_per_sample, uint32 *byte_size)
typedef PLATFORM_LOAD_AUDIO(Platform_Load_Audio);

#define PLATFORM_ALLOCATE_MEMORY(name) void *name(size_t size)
typedef PLATFORM_ALLOCATE_MEMORY(Platform_Allocate_Memory);

#define PLATFORM_PLAY_SOUND(name) void name(char *filename)
typedef PLATFORM_PLAY_SOUND(Platform_Play_Sound);

typedef int platform_thread_fn(void *data);
typedef void Platform_Eight_Async_Proc(platform_thread_fn *fn, void *data[8]);

struct Memory
{
    void *storage;
    uint32 size;
};

struct Game_Memory
{
    Memory permanent_memory;
    Memory transient_memory;

    bool32 is_initialized;

    Platform_Load_Image *platform_load_image;
    Platform_Free_Image *platform_free_image;
    Platform_Load_Audio *platform_load_audio;
    Platform_Allocate_Memory *platform_allocate_memory;
    Platform_Eight_Async_Proc *platform_eight_async_proc;
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
    int32 sample_count;
    
    uint32 byte_per_sample;
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
        bool32 R;
        bool32 Q;
        bool32 number[10];
    } keyboard;
    struct
    {
        real32 dx;
        real32 dy;
        bool32 down;
    } mouse;
    real32 dt_per_frame;
};

struct Debug_State
{
    real32 last_frame_process_time;
    real32 last_frame_time;
    uint64 last_frame_mtsc;
};

#define GAME_UPDATE_AND_RENDER(name) void name(Game_Memory *memory, Game_Input *input, Game_Offscreen_Buffer *buffer, Debug_State *debug_state)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render);
#define GAME_PROCESS_SOUND(name) void name(Game_Memory *memory, Game_Sound_Buffer *buffer)
typedef GAME_PROCESS_SOUND(Game_Process_Sound);
