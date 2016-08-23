#pragma once

#include "game_platform.h"

//
//
//

#include "game_math.h"

struct Projection_Spec
{
    real32 dim;
    real32 fov;
    real32 view_distance;
};

struct Reflection_Sample
{
    bool32 x_side_faced;
    real32 ray_length;
};

//NOTE(chen): each tile size must be exactly 1.0f unit in the game world
struct Tile_Map
{
    uint32 *tiles;
    uint32 exception_tile_value; //NOTE(chen): return value for out-of-bound query request
    int32 tile_count_x;
    int32 tile_count_y;
};

struct Game_State
{
    v2 player_position;
    real32 player_angle;
};
