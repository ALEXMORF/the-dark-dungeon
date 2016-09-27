#include "game_platform.h"

#include <stdio.h>
#include <intrin.h>
#include <windows.h>
#include <windowsx.h>

#pragma warning(push)
#pragma warning(disable: 4244)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma warning(pop)

#pragma intrinsic(__rdtsc)

#define show_error(message) MessageBoxA(0, message, "ERROR", MB_OK|MB_ICONERROR)

global_variable bool32 global_running;
global_variable LARGE_INTEGER global_performance_frequency;
global_variable WINDOWPLACEMENT global_window_position = { sizeof(global_window_position) };

global_variable bool32 global_app_is_active;
global_variable int32 global_previous_mouse_x;
global_variable int32 global_previous_mouse_y;
global_variable bool32 global_mouse_down;

struct Win32_Offscreen_Buffer
{
    int width;
    int height;
    int pitch;
    void *memory;
    BITMAPINFO info;
};
    
struct Game_Code
{
    Game_Update_And_Render *game_update_and_render;

    HMODULE library;
    FILETIME last_write_time;
    
    bool32 is_valid;
};

internal void
toggle_fullscreen(HWND window)
{
    DWORD style = GetWindowLong(window, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW) {
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(window, &global_window_position) &&
            GetMonitorInfo(MonitorFromWindow(window,
                                             MONITOR_DEFAULTTOPRIMARY), &mi)) {
            SetWindowLong(window, GWL_STYLE,
                          style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(window, HWND_TOP,
                         mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLong(window, GWL_STYLE,
                      style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &global_window_position);
        SetWindowPos(window, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal Win32_Offscreen_Buffer
win32_create_offscreen_buffer(int width, int height)
{
    Win32_Offscreen_Buffer result = {};

    int bytesPerPixel = 4;
    
    result.width = width;
    result.height = height;
    result.pitch = width * bytesPerPixel;

    result.info.bmiHeader.biSize = sizeof(result.info.bmiHeader);
    result.info.bmiHeader.biWidth = width;
    result.info.bmiHeader.biHeight = -height;
    result.info.bmiHeader.biPlanes = 1;
    result.info.bmiHeader.biBitCount = 32;
    result.info.bmiHeader.biCompression = BI_RGB;

    unsigned int buffer_memory_size = width * height * bytesPerPixel;
    result.memory = VirtualAlloc(0, buffer_memory_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    
    return result;
}

internal void
win32_display_offscreen_buffer(Win32_Offscreen_Buffer *buffer, HWND window)
{
    HDC device_context = GetDC(window);

    RECT client_rect;
    GetClientRect(window, &client_rect);
    int32 window_width = client_rect.right - client_rect.left;
    int32 window_height = client_rect.bottom - client_rect.top;

    StretchDIBits(device_context,
                  0, 0, window_width, window_height,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory, &buffer->info,
                  DIB_RGB_COLORS, SRCCOPY);
    
    ReleaseDC(window, device_context);
}

internal HWND
win32_create_window(const char *window_name, WNDPROC window_proc = DefWindowProc,
                    int window_width = CW_USEDEFAULT, int window_height = CW_USEDEFAULT,
                    int window_upper_left = -100, int window_upper_top = -100)
{
    HWND window_handle = 0;
    
    if (window_upper_left < 0 || window_upper_top < 0)
    {
        int screen_width = GetSystemMetrics(SM_CXSCREEN);
        int screen_height = GetSystemMetrics(SM_CYSCREEN);
        window_upper_left = (screen_width - window_width) / 4;
        window_upper_top = (screen_height - window_height) / 2; 
    }

    WNDCLASSA window_class = {};

    window_class.style = CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = window_proc;
    window_class.hCursor = LoadCursor(0, IDC_ARROW);
    window_class.hInstance = GetModuleHandle(0);
    window_class.lpszClassName = "window_class";

    if (RegisterClass(&window_class))
    {
        DWORD window_style = WS_OVERLAPPEDWINDOW|WS_VISIBLE;
        RECT window_rect = {0, 0, window_width, window_height};
        AdjustWindowRect(&window_rect, window_style, 0);
        
        window_handle = CreateWindowEx(0,
                                       window_class.lpszClassName,
                                       window_name,
                                       window_style,
                                       window_upper_left,
                                       window_upper_top,
                                       window_rect.right - window_rect.left,
                                       window_rect.bottom - window_rect.top,
                                       0, 0,
                                       GetModuleHandle(0),
                                       0);
    }

    return window_handle;
}

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

LRESULT CALLBACK
win32_main_window_callback(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    LRESULT result = 0;
    switch (message)
    {
        case WM_ACTIVATEAPP:
        {
            global_app_is_active = (w_param == TRUE);
        } break;

        case WM_LBUTTONDOWN:
        {
            global_mouse_down = true;
        } break;

        case WM_LBUTTONUP:
        {
            global_mouse_down = false;
        } break;
        
        case WM_MOUSEMOVE:
        {
            global_previous_mouse_x = GET_X_LPARAM(l_param);
            global_previous_mouse_y = GET_Y_LPARAM(l_param);
        } break;
        
        case WM_SETCURSOR:
        {
            SetCursor(0);
        } break;
        
        case WM_CLOSE:
        {
            global_running = false;
        } break;
        case WM_DESTROY:
        {
            show_error("WM_CLOSE leaked over to WM_DESTROY");
            global_running = false;
        } break;
        
        default:
        {
            result = DefWindowProc(window, message, w_param, l_param);
        } break;
    }
    return result;
}

int CALLBACK
WinMain(HINSTANCE h_instance, HINSTANCE h_prev_instance, LPSTR cmd_line, int cmd_show)
{
    int32 window_width = 960;
    int32 window_height = 540;
    int32 buffer_width = 960;
    int32 buffer_height = 540;    
    uint32 permanent_game_memory_size = megabytes(64);
    uint32 transient_game_memory_size = megabytes(128);
    int32 target_frame_per_second = 60;
    bool32 frame_rate_lock = true;

    HWND window = win32_create_window("Dark Dungeon", win32_main_window_callback,
                                      window_width, window_height);
    Win32_Offscreen_Buffer win32_buffer = win32_create_offscreen_buffer(buffer_width, buffer_height);
    
    QueryPerformanceFrequency(&global_performance_frequency);
    timeBeginPeriod(1);
    
    Game_Memory game_memory = {};
    game_memory.permanent_storage_size = permanent_game_memory_size;
    game_memory.transient_storage_size = transient_game_memory_size;
    uint32 total_game_memory_size = permanent_game_memory_size + transient_game_memory_size;
    game_memory.permanent_storage = VirtualAlloc(0, total_game_memory_size,
                                                 MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    game_memory.transient_storage = ((uint8 *)game_memory.permanent_storage +
                                     permanent_game_memory_size);
    
    game_memory.platform_load_image = stbi_load;
    game_memory.platform_free_image = stbi_image_free;
    game_memory.platform_allocate_memory = malloc;
    assert(game_memory.platform_load_image);
    assert(game_memory.platform_free_image);
    assert(game_memory.platform_allocate_memory);
    
    Game_Input game_input = {};
    Game_Offscreen_Buffer game_buffer = {};
    
    Game_Code game_code = {};
    win32_load_game_code(&game_code);

    if (!game_code.is_valid)
    {
        show_error("game code loading failure");
        return -1;
    }
    
    uint64 last_tsc = __rdtsc();
        
    LARGE_INTEGER last_counter = win32_get_wallclock();
    real32 ms_per_frame = 0.0f;
    
    global_running = true;
    while (global_running)
    {
        //NOTE(chen):check for latest DLL and load that thing
        FILETIME dll_last_write_time = win32_get_file_last_write_time("../build/game.dll");
        if (CompareFileTime(&dll_last_write_time, &game_code.last_write_time) != 0)
        {
            win32_unload_game_code(&game_code);
            win32_load_game_code(&game_code);
        }
            
        //handles input
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            switch (message.message)
            {
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYUP:
                case WM_KEYDOWN:
                {
                    WPARAM key_code = message.wParam;
                    bool32 key_down = ((message.lParam & (1 << 31)) == 0);
                    bool32 key_was_down = ((message.lParam & (1 << 30)) != 0);

                    if (key_down != key_was_down)
                    {
                        if (key_code == VK_ESCAPE)
                        {
                            global_running = false;
                        }

                        if (key_code == VK_F11 && key_down)
                        {
                            toggle_fullscreen(window);
                        }
                        
#define bind_key(win32_key, game_key) if (key_code == (win32_key)) game_key = key_down;
                        bind_key('A', game_input.keyboard.left);
                        bind_key('D', game_input.keyboard.right);
                        bind_key('W', game_input.keyboard.up);
                        bind_key('S', game_input.keyboard.down);
                        bind_key(VK_SPACE, game_input.keyboard.space);
                    }
                } break;

                default:
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                } break;
            }
        }

        /*NOTE(chen): calculate difference between mouse and middle of screen, 
          then lock the cursor back to center
          ps: do so only if game is the activated window
        */
        if (global_app_is_active)
        {
            RECT client_rect;
            GetClientRect(window, &client_rect);
            POINT screen_center = {(client_rect.right - client_rect.left)/2,
                                   (client_rect.bottom - client_rect.top)/2};
            game_input.mouse.dx = (real32)(global_previous_mouse_x - screen_center.x);
            game_input.mouse.dy = (real32)(global_previous_mouse_y - screen_center.y);
            
            ClientToScreen(window, &screen_center);         
            SetCursorPos(screen_center.x , screen_center.y);
        }

        game_input.mouse.down = global_mouse_down;
        game_input.dt_per_frame = ms_per_frame / 1000.0f;
        
        game_buffer.width = win32_buffer.width;
        game_buffer.height = win32_buffer.height;
        game_buffer.pitch = win32_buffer.pitch;
        game_buffer.memory = win32_buffer.memory;
        
        game_code.game_update_and_render(&game_memory, &game_input, &game_buffer);
        win32_display_offscreen_buffer(&win32_buffer, window);
        LARGE_INTEGER current_counter = win32_get_wallclock();
        real32 elapsed_ms = win32_get_elapsed_ms(last_counter, current_counter);
        real32 target_ms = 1000.0f / (real32)target_frame_per_second;
        real32 ms_took_to_process = elapsed_ms;

        uint64 current_tsc = __rdtsc();
        uint64 mtsc = (current_tsc - last_tsc) / 1024*1024;
        last_tsc = __rdtsc();

        if (frame_rate_lock)
        {
            if (target_ms > elapsed_ms)
            {
                Sleep((DWORD)(target_ms - elapsed_ms));
                do
                {
                    elapsed_ms = win32_get_elapsed_ms(last_counter, win32_get_wallclock());
                } while (elapsed_ms < target_ms);
            }
        }
        else
        {
            Sleep(2);
        }
        last_counter = win32_get_wallclock();   
        ms_per_frame = elapsed_ms;
        
#if POOR_DEBUG
        char debug_buffer[256] = {};
        snprintf(debug_buffer, sizeof(debug_buffer),
                 "ms_per_frame:%.02f, ms_took_to_process:%.02f, Mega-TSC:%lld \n",
                 elapsed_ms, ms_took_to_process, mtsc);
        OutputDebugStringA(debug_buffer);
#endif
    }

    return 0;
}
