#pragma once

#include "game_platform.h"

#define abs(value) ((value) > 0? (value): -(value))
#define swap(x, y) do {x ^= y; y ^= x; x ^= y;} while (0)
#define if_do(condition, action) do { if (condition) action; } while (0)
#define array_count(array) (sizeof(array)/sizeof(array[0]))

#include "game_math.h"
#include "game_tiles.h"

#include "game_asset.h"
#include "game_render.h"
#include "game_raycaster.h"

struct Linear_Allocator
{
    uint8 *base_ptr;
    uint32 size;
    uint32 used;
};

struct Wall_Textures
{
    Loaded_Image *E;
    int32 count;
};

struct Game_State
{
    //memory
    Linear_Allocator permanent_allocator;
    Linear_Allocator transient_allocator;
    
    //game
    v2 player_position;
    v2 barrel_position;
    real32 player_angle;
    Tile_Map tile_map;
    
    //asset
    Wall_Textures wall_textures;
    Loaded_Image floor_texture;
    Loaded_Image ceiling_texture;
    Loaded_Image barrel_texture;
    Loaded_Image pillar_texture;
    
    //render
    Render_Context render_context;
};
