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

#include "game_entity.h"
#include "game_simulate.h"
#include "game_ui.h"

#include "game_meta.h"

enum Weapon_Type
{
    knife,
    pistol,
    rifle,
    minigun,
    weapon_count
};

#define PLAYER_MAX_HP 10

struct Weapon
{
    Weapon_Type type;
    int32 animation_index;
    real32 cd;
    real32 cd_counter;
    
    bool32 is_reloading;
    real32 max_reload_time;
    real32 reload_time;
    int32 cache_max_ammo;
    int32 cache_ammo;
    int32 bank_ammo;
};

struct Player
{
    //coordniate
    v2 position;
    v2 velocity;
    real32 collision_radius;
    real32 angle;
    real32 pace;
    
    //NOTE(chen): for weapon animation
    real32 weapon_reload_offset;
    
    //record
    bool32 has_fired;
    int32 hp;

    //components
    Weapon weapon;
};

struct Game_State
{
    //memory
    Linear_Allocator permanent_allocator;
    Linear_Allocator transient_allocator;
    
    //game
    Player player;
    Tile_Map tile_map;
    DBuffer(Entity) entity_buffer;
    
    //bitmap asset
    DBuffer(Loaded_Image) wall_texture_buffer;
    Loaded_Image floor_texture;
    Loaded_Image ceiling_texture;
    Loaded_Image barrel_texture;
    Loaded_Image pillar_texture;
    Loaded_Image light_texture;
    Loaded_Image_Sheet guard_texture_sheet;
    Loaded_Image_Sheet ss_texture_sheet;
    Loaded_Image_Sheet weapon_texture_sheet;
    
    //font
    Loaded_Image_Sheet font_bitmap_sheet;
    
    //audio asset
    Loaded_Audio pistol_sound;
    Loaded_Audio pistol2_sound;
    Loaded_Audio pistol_reload_sound;
    Loaded_Audio rifle_sound;
    Loaded_Audio minigun_sound;
    Loaded_Audio background_music;
    
    //render
    Render_Context render_context;
    
    //audio system
    Audio_System audio_system;
    
    //HUD persistent data
    real32 hp_display_width;
};
