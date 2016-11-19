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

#if 0
//NOTE(chen): structure for parallel rendering
struct Render_Data
{
    Game_Offscreen_Buffer *buffer;
    Render_Context *render_context;
    Tile_Map *tile_map;
    v2 position;
    real32 view_angle;
    Loaded_Image *floor_texture;
    Loaded_Image *ceiling_texture;
    DBuffer(Loaded_Image) *wall_textures;
    Projection_Spec *projection_spec;
    World_Spec *world_spec;

    int current_thread_index;
    int thread_count;
};
#endif
