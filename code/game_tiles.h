#pragma once

enum Tile_Type
{
    TILE_TYPE_ZERO,
    TILE_TYPE_REDBRICK,
    TILE_TYPE_BLUESTONE,
    TILE_TYPE_COLORSTONE,
    TILE_TYPE_EAGLE,
    TILE_TYPE_PURPLESTONE,
    TILE_TYPE_WOOD,
    TILE_TYPE_MOSSY,

    TILE_TYPE_COUNT
};
    
//NOTE(chen): each tile size must be exactly 1.0f unit in the game world
struct Tile_Map
{
    uint32 *tiles;
    uint32 exception_tile_value; //NOTE(chen): return value for out-of-bound query request
    int32 tile_count_x;
    int32 tile_count_y;
};
