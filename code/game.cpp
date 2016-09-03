#include "game.h"

#include "game_math.cpp"
#include "game_tiles.cpp"

#include "game_asset.cpp"
#include "game_render.cpp"
#include "game_raycaster.cpp"


#define Copy_Array(source, dest, count, type) copy_memory(source, dest, count*sizeof(type))
inline void
copy_memory(void *source_in, void *dest_in, uint32 size)
{
    uint8 *source = (uint8 *)source_in;
    uint8 *dest = (uint8 *)dest_in;
    for (uint32 i = 0; i < size; ++i)
    {
	dest[i] = source[i];
    }
}

inline void
initialize_linear_allocator(Linear_Allocator *allocator, void *base_ptr, uint32 size)
{
    allocator->base_ptr = (uint8 *)base_ptr;
    allocator->size = size;
    allocator->used = 0;
}

#define Push_Array(allocator, wanted_size, type) (type *)linear_allocate(allocator, wanted_size*sizeof(type))
inline void *
linear_allocate(Linear_Allocator *allocator, uint32 wanted_size)
{
    assert(allocator->used + wanted_size <= allocator->size);
    void *result = (uint8 *)allocator->base_ptr + allocator->used;
    allocator->used += wanted_size;
    return result;
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    assert(sizeof(Game_State) <= memory->permanent_storage_size);
    Game_State *game_state = (Game_State *)memory->permanent_storage;
    
    if (!memory->is_initialized)
    {
	//prepare memory allocators
	initialize_linear_allocator(&game_state->permanent_allocator,
				    (uint8 *)memory->permanent_storage + sizeof(Game_State),
				    memory->permanent_storage_size - sizeof(Game_State));
	initialize_linear_allocator(&game_state->transient_allocator,
				    (uint8 *)memory->transient_storage,
				    memory->transient_storage_size);
	
	//game
//TODO(chen): this is some improv tile-map init code, replace this with procedural generation later
#define map_width 20
#define map_height 20
	uint32 tiles[map_width*map_height] =
	    {
		3, 3, 3, 3, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1,
		3, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		3, 0, 0, 0, 4, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		3, 0, 0, 0, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
		3, 1, 4, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 0, 0, 2, 1, 1, 1,
		1, 1, 1, 2, 0, 0, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
		4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1
	    };
	Tile_Map *tile_map = &game_state->tile_map;
	tile_map->tile_count_x = map_width;
	tile_map->tile_count_y = map_height;
	tile_map->exception_tile_value = 1;
	int32 tile_count = tile_map->tile_count_x * tile_map->tile_count_y;
	tile_map->tiles = Push_Array(&game_state->permanent_allocator, tile_count, uint32);
	Copy_Array(tiles, tile_map->tiles, tile_count, uint32);

	//init all game states
	game_state->player_position = {3.0f, 3.0f};
	game_state->player_angle = 0.0f;
	game_state->barrel_position = {5.0f, 5.0f};
	    
	//load all assets
#define Load_Wall_Tex(index, filename) game_state->wall_textures.E[index] = load_image(memory->platform_load_image, filename);	
	game_state->wall_textures.count = 6;
	game_state->wall_textures.E = Push_Array(&game_state->permanent_allocator,
						 game_state->wall_textures.count,
						 Loaded_Image);
	Load_Wall_Tex(0, "../data/redbrick.png");
	Load_Wall_Tex(1, "../data/bluestone.png");
	Load_Wall_Tex(2, "../data/colorstone.png");
	Load_Wall_Tex(3, "../data/eagle.png");
	Load_Wall_Tex(4, "../data/purplestone.png");
	Load_Wall_Tex(5, "../data/wood.png");
	
	game_state->floor_texture = load_image(memory->platform_load_image, "../data/greystone.png");
	game_state->ceiling_texture = load_image(memory->platform_load_image, "../data/greystone.png");
	game_state->barrel_texture = load_image(memory->platform_load_image, "../data/barrel.png");
	game_state->pillar_texture = load_image(memory->platform_load_image, "../data/pillar.png");

	//safety checking all assets are there
	for (int32 i = 0; i < game_state->wall_textures.count; ++i)
	{
	    assert(game_state->wall_textures.E[i].data);
	}
	assert(game_state->floor_texture.data);
	assert(game_state->ceiling_texture.data);
	assert(game_state->barrel_texture.data);

	//prepare renderer context
	Render_Context *render_context = &game_state->render_context;
	
	render_context->z_buffer = Push_Array(&game_state->permanent_allocator,
					      buffer->width, real32);
	render_context->floorcast_table_count = buffer->height/2;
	render_context->floorcast_table = Push_Array(&game_state->permanent_allocator,
						     render_context->floorcast_table_count, real32);
	for (int32 i = 0; i < render_context->floorcast_table_count; ++i)
	{
	    real32 inverse_aspect_ratio = (real32)buffer->width / buffer->height;
	    real32 real_scan_y = ((real32)buffer->height/2 + (real32)i / inverse_aspect_ratio);
	    render_context->floorcast_table[i] = ((real32)buffer->height /
						  (2*real_scan_y - buffer->height));
	}

	memory->is_initialized = true;
    }

    //NOTE(chen): transient memory gets dumped every frame
    game_state->transient_allocator.used = 0;
    
    //
    //
    //

    real32 forward = 0.0f;
    real32 left = 0.0f;
    if (input->keyboard.left)
    {
	left = 1.0f;
    }
    if (input->keyboard.right)
    {
	left = -1.0f;
    }
    if (input->keyboard.up)
    {
	forward = 1.0f;
    }
    if (input->keyboard.down)
    {
	forward = -1.0f;
    }
    
    v2 player_delta_direction = {};
    player_delta_direction.y += sinf(game_state->player_angle) * forward;
    player_delta_direction.x += cosf(game_state->player_angle) *forward;
    player_delta_direction.y += sinf(game_state->player_angle + pi32/2.0f) * left;
    player_delta_direction.x += cosf(game_state->player_angle + pi32/2.0f) * left;
    player_delta_direction = normalize(player_delta_direction);
    
    real32 player_speed = 1.5f;
    player_delta_direction *= player_speed;
    game_state->player_position += player_delta_direction * input->dt_per_frame;
    
    //
    //
    //

    real32 mouse_sensitivity = 0.7f;
    real32 player_delta_angle = -input->mouse.dx / 500.0f * pi32/3.0f * mouse_sensitivity;
    game_state->player_angle += player_delta_angle;
    recanonicalize_angle(&game_state->player_angle);
    
    //
    //
    //

    fill_buffer(buffer, 0);
    
    Sprite sprite_list[1] =
	{
	    {{1.0f, 1.0f}, {5.0f, 5.0f}, &game_state->barrel_texture} 
	};
    
    render_3d_scene(buffer, &game_state->render_context, &game_state->tile_map,
		    game_state->player_position, game_state->player_angle,
		    &game_state->floor_texture, &game_state->ceiling_texture,
		    &game_state->wall_textures,
		    sprite_list, array_count(sprite_list));
}
