#pragma once

//NOTE(chen): each tile size must be exactly 1.0f unit in the game world
struct Tile_Map
{
    uint32 *tiles;
    uint32 exception_tile_value; //NOTE(chen): return value for out-of-bound query request
    int32 tile_count_x;
    int32 tile_count_y;
};
