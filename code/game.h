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

#include "game_asset.h"
#include "game_render.h"
#include "game_sprite.h"
#include "game_raycaster.h"

#include "game_entity.h"
#include "game_simulate.h"

struct Linear_Allocator
{
    uint8 *base_ptr;
    uint32 size;
    uint32 used;
};

struct Texture_List
{
    Loaded_Image *E;
    int32 count;
};

enum Weapon_Type
{
    knife,
    pistol,
    rifle,
    minigun,
    weapon_count
};

struct Player
{
    //coordniate
    v2 position;
    v2 velocity;
    real32 collision_radius;
    real32 angle;
    real32 pace;

    //record
    bool32 has_fired;
    
    //weapon handling
    Weapon_Type weapon;
    int32 weapon_animation_index;
    real32 weapon_cd;
    real32 weapon_cd_counter;
};

struct Game_State
{
    //memory
    Linear_Allocator permanent_allocator;
    Linear_Allocator transient_allocator;
    
    //game
    Player player;
    Tile_Map tile_map;
    Entity_List entity_list;
    Entity *currently_aimed_entity;
    
    //asset
    Texture_List wall_textures;
    Loaded_Image floor_texture;
    Loaded_Image ceiling_texture;
    Loaded_Image barrel_texture;
    Loaded_Image pillar_texture;
    Loaded_Image light_texture;
    Loaded_Image_Sheet guard_texture_sheet;
    Loaded_Image_Sheet ss_texture_sheet;
    Loaded_Image_Sheet weapon_texture_sheet;
    
    //render
    Render_Context render_context;
};
