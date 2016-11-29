#include "game_platform.h"

#include <stdio.h>
#include <math.h>
#include <intrin.h>
#include <windows.h>
#include <windowsx.h>
#include <SDL.h>

#pragma warning(push)
#pragma warning(disable: 4244)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma warning(pop)

#pragma intrinsic(__rdtsc)

#define show_error(message) MessageBoxA(0, message, "ERROR", MB_OK|MB_ICONERROR)

global_variable LARGE_INTEGER global_performance_frequency;
global_variable SDL_Thread *global_threads[8];

struct Game_Code
{
    Game_Update_And_Render *game_update_and_render;
    Game_Process_Sound *game_process_sound;

    HMODULE library;
    FILETIME last_write_time;
    
    bool32 is_valid;
};

internal FILETIME
win32_get_file_last_write_time(const char *filename)
{
    FILETIME file_time = {};
    HANDLE file_handle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    GetFileTime(file_handle, 0, 0, &file_time);
    CloseHandle(file_handle);
        
    return file_time;
}

internal void
win32_load_game_code(Game_Code *game_code)
{
    const char *dll_path = "../build/game.dll";
    const char *temporary_dll_path = "../build/game_temp.dll";
    
    game_code->last_write_time = win32_get_file_last_write_time(dll_path);
    CopyFile(dll_path, temporary_dll_path, FALSE);

    HMODULE library = LoadLibrary(temporary_dll_path);
    if (library)
    {
        game_code->game_update_and_render = (Game_Update_And_Render *)
            GetProcAddress(library, "game_update_and_render");
        game_code->game_process_sound = (Game_Process_Sound *)
            GetProcAddress(library, "game_process_sound");
        game_code->library = library;

        game_code->is_valid = true;
    }
    else
    {
        show_error("failed to load game code");
    }
}

internal void
win32_unload_game_code(Game_Code *game_code)
{
    game_code->is_valid = false;
    game_code->game_update_and_render = 0;
    FreeLibrary(game_code->library);
}

inline LARGE_INTEGER
win32_get_wallclock()
{
    LARGE_INTEGER Result = {};
    QueryPerformanceCounter(&Result);
    return Result;
}

internal real32
win32_get_elapsed_ms(LARGE_INTEGER start, LARGE_INTEGER end)
{
    real32 elapsed_ms = 1000.0f * ((real32)(end.QuadPart - start.QuadPart)/
                                   (real32)(global_performance_frequency.QuadPart));
    return elapsed_ms;
}

internal void
sdl_initialize_audio(int32 samples_per_second, uint16 buffer_size)
{
    SDL_AudioSpec sdl_audio_spec = {};
    sdl_audio_spec.freq = samples_per_second;
    sdl_audio_spec.format = AUDIO_S16LSB;
    sdl_audio_spec.channels = 2;
    sdl_audio_spec.samples = buffer_size;
    
    SDL_OpenAudio(&sdl_audio_spec, 0);
}

internal void *
sdl_load_audio(char *filename, int *channels, int *byte_per_sample, uint32 *byte_size)
{
    uint8 *audio_memory = 0;  
    SDL_AudioSpec sdl_audio_spec = {};
    SDL_LoadWAV(filename, &sdl_audio_spec, &audio_memory, byte_size);

    *channels = sdl_audio_spec.channels;
    switch (sdl_audio_spec.format)
    {
        case AUDIO_S8:
        case AUDIO_U8:
        {
            *byte_per_sample = 1;
        } break;
        case AUDIO_S16LSB:
        case AUDIO_S16MSB:
        case AUDIO_U16LSB:
        case AUDIO_U16MSB:
        {
            *byte_per_sample = 2;
        } break;
        case AUDIO_S32LSB:
        case AUDIO_S32MSB:
        case AUDIO_F32LSB:
        case AUDIO_F32MSB:
        {
            *byte_per_sample = 4;
        } break;
        default:
        {
            assert(!"unknown audio format");
        };
    }
        
    return audio_memory;
}

void eight_async_proc(platform_thread_fn *fn, void *data[8])
{
    global_threads[0] = SDL_CreateThread(fn, "thread_0", data[0]);
    global_threads[1] = SDL_CreateThread(fn, "thread_1", data[1]);
    global_threads[2] = SDL_CreateThread(fn, "thread_2", data[2]);
    global_threads[3] = SDL_CreateThread(fn, "thread_3", data[3]);
    global_threads[4] = SDL_CreateThread(fn, "thread_4", data[4]);
    global_threads[5] = SDL_CreateThread(fn, "thread_5", data[5]);
    global_threads[6] = SDL_CreateThread(fn, "thread_6", data[6]);
    global_threads[7] = SDL_CreateThread(fn, "thread_7", data[7]);
    for (int i = 0; i < 8; ++i)
    {
        SDL_WaitThread(global_threads[i], 0);
        global_threads[i] = 0;
    }
}

int CALLBACK
WinMain(HINSTANCE h_instance, HINSTANCE h_prev_instance, LPSTR cmd_line, int cmd_show)
{
    int32 window_width = 960;
    int32 window_height = 540;
    int32 buffer_width = 960;
    int32 buffer_height = 540;
    int32 audio_sample_frequency = 48000;
    uint32 permanent_game_memory_size = megabytes(64);
    uint32 transient_game_memory_size = megabytes(128);
    int32 target_frame_per_second = 60;
    real32 audio_latency_ms = (1000.0f / target_frame_per_second) + 10; //in ms
    bool32 frame_rate_lock = true;
    
    //NOTE(chen): limit windows to schedule this program 1ms at max
    timeBeginPeriod(1);
    
    SDL_Init(SDL_INIT_EVERYTHING);
    
    SDL_version sdl_compiled_version = {};
    SDL_version sdl_linked_version = {};
    SDL_VERSION(&sdl_compiled_version);
    SDL_GetVersion(&sdl_linked_version);
    if (sdl_linked_version.major != sdl_compiled_version.major ||
        sdl_linked_version.minor != sdl_compiled_version.minor ||
        sdl_linked_version.patch != sdl_compiled_version.patch)
    {
        show_error("your SDL runtime binary and SDL static library have different versions, play at your own caution");
    }
    
    SDL_Window *sdl_window = SDL_CreateWindow("The Dark Dungeon",
                                              SDL_WINDOWPOS_CENTERED,
                                              SDL_WINDOWPOS_CENTERED,
                                              window_width, window_height,
                                              0);
#if VSYNC
    SDL_Renderer *sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_ACCELERATED);
#else
    SDL_Renderer *sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
#endif
    
    SDL_Texture *sdl_offscreen_texture = SDL_CreateTexture(sdl_renderer,
                                                           SDL_PIXELFORMAT_ARGB8888,
                                                           SDL_TEXTUREACCESS_STREAMING,
                                                           buffer_width, buffer_height);
    
    int32 sample_count_per_frame = (int32)(audio_sample_frequency * (audio_latency_ms / 1000.0f));
    bool32 sound_is_playing = false;
    sdl_initialize_audio(audio_sample_frequency, (uint16)sample_count_per_frame);
    
    QueryPerformanceFrequency(&global_performance_frequency);
    timeBeginPeriod(1);
    
    Game_Memory game_memory = {};
    game_memory.permanent_memory.size = permanent_game_memory_size;
    game_memory.transient_memory.size = transient_game_memory_size;
    uint32 total_game_memory_size = permanent_game_memory_size + transient_game_memory_size;
    game_memory.permanent_memory.storage = VirtualAlloc(0, total_game_memory_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    game_memory.transient_memory.storage = ((uint8 *)game_memory.permanent_memory.storage + permanent_game_memory_size);
    game_memory.platform_load_image = stbi_load;
    game_memory.platform_free_image = stbi_image_free;
    game_memory.platform_load_audio = sdl_load_audio;
    game_memory.platform_allocate_memory = malloc;
    game_memory.platform_eight_async_proc = eight_async_proc;
    assert(game_memory.platform_load_image);
    assert(game_memory.platform_free_image);
    assert(game_memory.platform_load_audio);
    assert(game_memory.platform_allocate_memory);
    assert(game_memory.platform_eight_async_proc);
    
    Game_Input game_input = {};

    Game_Offscreen_Buffer game_buffer = {};
    uint32 bytes_per_pixel = 4;
    game_buffer.width = buffer_width;
    game_buffer.height = buffer_height;
    game_buffer.pitch = buffer_width * bytes_per_pixel;
    game_buffer.memory = VirtualAlloc(0, game_buffer.width * game_buffer.height * bytes_per_pixel, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

    Game_Sound_Buffer game_sound_buffer = {};
    game_sound_buffer.sample_count = sample_count_per_frame;
    game_sound_buffer.byte_per_sample = 16 / 8;
    int32 channel_count = 2;
    game_sound_buffer.memory = VirtualAlloc(0, game_sound_buffer.sample_count * game_sound_buffer.byte_per_sample * channel_count, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    
    Game_Code game_code = {};
    win32_load_game_code(&game_code);
    if (!game_code.is_valid)
    {
        show_error("game code loading failure");
        return -1;
    }

    Debug_State debug_state = {};
    
    uint64 last_tsc = __rdtsc();
        
    LARGE_INTEGER last_counter = win32_get_wallclock();
    real32 ms_per_frame = 0.0f;

    bool32 window_is_active = true;
    bool32 is_fullscreen = false;
    bool32 game_running = true;
    while (game_running)
    {
        //NOTE(chen):check for latest DLL and load that thing
        FILETIME dll_last_write_time = win32_get_file_last_write_time("../build/game.dll");
        if (CompareFileTime(&dll_last_write_time, &game_code.last_write_time) != 0)
        {
            win32_unload_game_code(&game_code);
            win32_load_game_code(&game_code);
        }

        SDL_ShowCursor(0);
        real32 mouse_dx = 0, mouse_dy = 0;
        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event))
        {
            switch (sdl_event.type)
            {
                case SDL_QUIT:
                {
                    game_running = false;
                } break;

                case SDL_KEYDOWN:
                case SDL_KEYUP:
                {
                    auto key_code = sdl_event.key.keysym.sym;
                    auto *keyboard = &game_input.keyboard;
                    
#define bind_key(sdl_key, game_key) if (key_code == sdl_key) game_key = sdl_event.key.state == SDL_PRESSED;
                    bind_key(SDLK_a, keyboard->left);
                    bind_key(SDLK_d, keyboard->right);
                    bind_key(SDLK_w, keyboard->up);
                    bind_key(SDLK_s, keyboard->down);
                    bind_key(SDLK_r, keyboard->R);

                    bind_key(SDLK_1, keyboard->number[1]);
                    bind_key(SDLK_2, keyboard->number[2]);
                    bind_key(SDLK_3, keyboard->number[3]);
                    bind_key(SDLK_4, keyboard->number[4]);
                    bind_key(SDLK_5, keyboard->number[5]);
                    bind_key(SDLK_6, keyboard->number[6]);
                    bind_key(SDLK_7, keyboard->number[7]);
                    bind_key(SDLK_8, keyboard->number[8]);
                    bind_key(SDLK_9, keyboard->number[9]);
                    bind_key(SDLK_0, keyboard->number[0]);
                    
                    if (key_code == SDLK_ESCAPE)
                    {
                        game_running = false;
                    }
                    if (key_code == SDLK_F11 && sdl_event.key.state == SDL_PRESSED)
                    {
                        if (is_fullscreen)
                        {
                            SDL_SetWindowFullscreen(sdl_window, 0);
                            is_fullscreen = false;
                        }
                        else
                        {
                            SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                            is_fullscreen = true;
                        }
                    }
                } break;
                
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEBUTTONDOWN:
                {
                    game_input.mouse.down = sdl_event.button.state  == SDL_PRESSED;
                } break;

                case SDL_MOUSEMOTION:
                {
                    mouse_dx = (real32)sdl_event.motion.x - (real32)window_width/2;
                    mouse_dy = (real32)sdl_event.motion.y - (real32)window_height/2;
                } break;

                case SDL_WINDOWEVENT:
                {
                    switch (sdl_event.window.event)
                    {
                        case SDL_WINDOWEVENT_ENTER:
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                        {
                            window_is_active = true;
                        } break;

                        case SDL_WINDOWEVENT_LEAVE:
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                        {
                            window_is_active = false;
                        } break;
                    }
                } break;
            }
        }
        game_input.mouse.dx = mouse_dx;
        game_input.mouse.dy = mouse_dy;
        game_input.dt_per_frame = ms_per_frame / 1000.0f;
        
        uint32 bytes_to_write = game_sound_buffer.sample_count * 2 * game_sound_buffer.byte_per_sample;
        
        //NOTE(chen): determine if skip current frame's sound to clear the latency
        bool skip_current_sound_frame = false;
        if (SDL_GetQueuedAudioSize(1) > bytes_to_write)
        {
            skip_current_sound_frame = true;
        }
        
        if (window_is_active)
        {
            SDL_WarpMouseInWindow(sdl_window, window_width/2, window_height/2);
            game_code.game_update_and_render(&game_memory, &game_input, &game_buffer, &debug_state);

            if (!skip_current_sound_frame)
            {
                game_code.game_process_sound(&game_memory, &game_sound_buffer);
                SDL_QueueAudio(1, game_sound_buffer.memory, bytes_to_write);
                if (!sound_is_playing)
                {
                    SDL_PauseAudio(0);
                    sound_is_playing = true;
                }
            }
        }

        SDL_UpdateTexture(sdl_offscreen_texture, 0, (uint32 *)game_buffer.memory, game_buffer.pitch);
        SDL_RenderClear(sdl_renderer);
        SDL_RenderCopy(sdl_renderer, sdl_offscreen_texture, 0, 0);

        real32 ms_took_to_process = win32_get_elapsed_ms(last_counter, win32_get_wallclock());
        uint64 current_tsc = __rdtsc();
        uint64 mtsc = (current_tsc - last_tsc) / 1024*1024;
        last_tsc = __rdtsc();
        
        SDL_RenderPresent(sdl_renderer);
        
        real32 elapsed_ms = win32_get_elapsed_ms(last_counter, win32_get_wallclock());
        real32 target_ms = 1000.0f / (real32)target_frame_per_second;
        
        if (frame_rate_lock)
        {
            if (target_ms > elapsed_ms)
            {
                Sleep((DWORD)(target_ms - elapsed_ms));
                elapsed_ms = win32_get_elapsed_ms(last_counter, win32_get_wallclock());
                while (elapsed_ms < target_ms)
                {
                    elapsed_ms = win32_get_elapsed_ms(last_counter, win32_get_wallclock());
                }
            }
        }
        else
        {
            Sleep(2);
        }
        last_counter = win32_get_wallclock();   
        ms_per_frame = elapsed_ms;

        debug_state.last_frame_time = ms_per_frame;
        debug_state.last_frame_process_time = ms_took_to_process;
        debug_state.last_frame_mtsc = mtsc;
    }
    SDL_PauseAudio(1);
    
    //NOTE(chen):only builds with SDL2.0.5, purely for asthetic bullshit
    if (sdl_compiled_version.major >= 2 && sdl_linked_version.major >= 2 &&
        sdl_compiled_version.minor >= 0 && sdl_linked_version.minor >= 0 &&
        sdl_compiled_version.patch >= 5 && sdl_linked_version.patch >= 5)
    {
        real32 window_opacity = 1.0f;
        real32 window_opacity_dec_rate = 0.05f;
        while (window_opacity > window_opacity_dec_rate)
        {
            window_opacity -= window_opacity_dec_rate;
            SDL_SetWindowOpacity(sdl_window, window_opacity);
            Sleep(20);
        }
    }
    
    SDL_CloseAudio();
    SDL_DestroyWindow(sdl_window);
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyTexture(sdl_offscreen_texture);
    SDL_Quit();
    return 0;
}
