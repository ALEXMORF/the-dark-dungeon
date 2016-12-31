#include "game_tiles.h"

inline uint32&
get_tile_value(Tile_Map *tile_map, int32 x, int32 y)
{
    if (x >= 0 && x < tile_map->tile_count_x && y >= 0 && y < tile_map->tile_count_y)
    {
        return tile_map->tiles[x + (tile_map->tile_count_y-y-1)*tile_map->tile_count_x];
    }
    else
    {
        return tile_map->exception_tile_value;
    }
}
