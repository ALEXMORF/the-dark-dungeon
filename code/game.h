#pragma once

#include "game_platform.h"

#define abs(value) ((value) > 0? (value): -(value))
#define swap(x, y) do {x ^= y; y ^= x; x ^= y;} while (0)
#define if_do(condition, action) do { if (condition) action; } while (0)
#define array_count(array) (sizeof(array)/sizeof(array[0]))

#include "game_random.h"
#include "game_constraint.h"
#include "game_math.h"
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

struct Game_State
{
    //memory
    Linear_Allocator permanent_allocator;
    Linear_Allocator transient_allocator;
    
    //game
    Player player;
    Tile_Map tile_map;
    DBuffer(Entity) entity_buffer;
    
    //system
    Game_Asset asset;
    Render_Context render_context;
    Audio_System audio_system;
    
    real32 hud_effect_last_time;
    uint32 hud_effect_color;
    real32 hp_display_width;
};
