#pragma once

struct Projection_Spec
{
    real32 dim;
    real32 fov;
    real32 view_distance;
};

struct World_Spec
{
    real32 wall_height;
    real32 wall_width;
};

struct Reflection_Sample
{
    bool32 x_side_faced;
    int32 tile_x;
    int32 tile_y;
    
    v2 hit_position;
    real32 ray_length;
    
    bool32 is_valid;
};

struct Render_Context
{
    real32 *floorcast_table;
    int32 floorcast_table_count;
    real32 *z_buffer;
};

struct Sprite
{
    v2 size;
    v2 position;
    Loaded_Image *texture;

    real32 distance_squared;
};
