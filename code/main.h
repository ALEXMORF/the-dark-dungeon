#pragma once

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
