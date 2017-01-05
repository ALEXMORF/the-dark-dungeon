#pragma once

#include "game_platform.h"

#define abs(value) ((value) > 0? (value): -(value))
#define swap(x, y) do {x ^= y; y ^= x; x ^= y;} while (0)
#define if_do(condition, action) do { if (condition) action; } while (0)
#define array_count(array) (sizeof(array)/sizeof(array[0]))
#define loop(count) for (int32 i = 0; i < count; ++i)
#define loop_for(it_name, count) for (int32 it_name = 0; it_name < count; ++it_name)
#define for_each(it_name, array) for (int32 it_name = 0; it_name < array_count(array); ++it_name)
#define wrap_array_index(index, array) index %= array_count(array)

inline void
toggle(bool32 *value)
{
    *value = !*value;
}
    
#include "game_math.h"
#include "game_random.h"
#include "game_constraint.h"
#include "game_tiles.h"
#include "game_memory.h"

#include "game_asset.h"
#include "game_render.h"
#include "game_sprite.h"
#include "game_raycaster.h"
#include "game_audio.h"

#include "game_simulate.h"
#include "game_player.h"
#include "game_entity.h"
#include "game_ui.h"

#include "game_meta.h"

#include "game_world.h"

struct Game_Asset
{
    DBuffer(Loaded_Image) wall_texture_buffer;
    
    Loaded_Image floor_texture;
    Loaded_Image ceiling_texture;
    Loaded_Image_Sheet guard_texture_sheet;
    Loaded_Image_Sheet ss_texture_sheet;
    Loaded_Image_Sheet weapon_texture_sheet;
    Loaded_Image_Sheet entity_sheet;
    
    Loaded_Image_Sheet font_bitmap_sheet;
    
    Loaded_Audio pistol_sound;
    Loaded_Audio pistol2_sound;
    Loaded_Audio pistol_reload_sound;
    Loaded_Audio rifle_sound;
    Loaded_Audio minigun_sound;
    Loaded_Audio background_music;
};

#define HUD_EFFECT_LAST_TIME 0.2f

struct Screen_Fader
{
    real32 time_left;
    real32 time_map_alpha;
    uint32 color;
};

struct Game_Over_State
{
    bool32 is_initialized;
    real32 timer;
};

struct Game_State
{
    Screen_Fader fader;
    
    //memory
    Linear_Allocator permanent_allocator;
    Linear_Allocator transient_allocator;
    
    //game
    World world;
    bool32 game_over;
    Game_Over_State game_over_state;
    
    //system
    Game_Asset asset;
    Render_Context render_context;
    Audio_System audio_system;

    //debug
    bool32 debug_hud_is_on;
    real32 hud_effect_last_time;
    uint32 hud_effect_color;
    real32 hp_display_width;
};
