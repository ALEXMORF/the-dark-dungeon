#pragma once

enum Room_Type
{
    ROOM_TYPE_BEGIN_ROOM,
    ROOM_TYPE_GUARD_ROOM,
    ROOM_TYPE_SS_ROOM,    
    ROOM_TYPE_SUPPLY_ROOM,
    ROOM_TYPE_TORTURE_ROOM,
    ROOM_TYPE_HEAVY_GUARD_ROOM,

    ROOM_TYPE_COUNT,
};

struct Wrapped_V2_Array
{
    v2 *e;
    int32 length;
    int32 current_index;
    
    v2 get_next();
};

struct World
{
    Tile_Map tile_map;
    Player player;
    DBuffer(Entity) entity_buffer;
};

struct Rect
{
    v2i min, max;

    bool32 collides(Rect *other_rect, int32 min_dist);
};

#define MAX_ROOM 500
#define TILE_VALUE_FILLER 1
struct Tile_Map_Generator
{
    int32 x_count, y_count;
    uint32 *tiles;
    Tile_Map *tile_map;
    
    Rect rooms[MAX_ROOM];
    int32 room_count;
    uint32 region_id;
    
    bool32 is_tile_valid(v2i tile_position);
    bool32 is_tile_walkable(v2i tile_position);

    void run(Tile_Map *tile_map, Linear_Allocator *linear_allocator);

    void unify_region_id(v2i tile_position, uint32 region_id);
    void flood_fill(v2i tile_position, v2i flood_direction);
    void uncarve(v2i tile_position);
};

