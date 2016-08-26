#include "game.h"

#define array_count(array) (sizeof(array)/sizeof(array[0]))

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

#define Allocator_Push_Array(allocator, wanted_size, type) (type *)linear_allocate(allocator, wanted_size*sizeof(type))
inline void *
linear_allocate(Linear_Allocator *allocator, uint32 wanted_size)
{
    assert(allocator->used + wanted_size <= allocator->size);
    void *result = allocator->base_ptr;
    allocator->base_ptr += wanted_size;
    allocator->used += wanted_size;
    return result;
}
		
extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    assert(sizeof(Game_State) <= memory->permanent_storage_size);
    Game_State *game_state = (Game_State *)memory->permanent_storage;
    
    if (!memory->is_initialized)
    {
	//memory
	initialize_linear_allocator(&game_state->permanent_allocator,
				    (uint8 *)memory->permanent_storage + sizeof(Game_State),
				    memory->permanent_storage_size - sizeof(Game_State));

	//game
	game_state->player_position = {3.0f, 3.0f};
	game_state->player_angle = 0.0f;

#define map_width 10
#define map_height 10
	uint32 tiles[map_width*map_height] =
	    {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
		1, 0, 0, 1, 0, 0, 1, 1, 1, 1,
		1, 0, 0, 0, 0, 0, 0, 1, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	    };
	Tile_Map *tile_map = &game_state->tile_map;
	tile_map->tile_count_x = 10;
	tile_map->tile_count_y = 10;
	tile_map->exception_tile_value = 1;
	int32 tile_count = tile_map->tile_count_x * tile_map->tile_count_y;
	tile_map->tiles = Allocator_Push_Array(&game_state->permanent_allocator,
					       tile_count,
					       uint32);
	Copy_Array(tiles, tile_map->tiles, tile_count, uint32);
	
	//asset
	game_state->wall_texture = load_image(memory->platform_load_image,
					      "../data/redbrick.png");
	game_state->floor_texture = load_image(memory->platform_load_image,
					       "../data/greystone.png");
	game_state->ceiling_texture = load_image(memory->platform_load_image,
						 "../data/greystone.png");
	assert(game_state->wall_texture.data);
	assert(game_state->floor_texture.data);
	assert(game_state->ceiling_texture.data);

	//floorcast lookup table
	int32 table_count = buffer->height/2;
	game_state->floorcast_table = Allocator_Push_Array(&game_state->permanent_allocator,
							   table_count, real32);	
	for (int32 i = 0; i < table_count; ++i)
	{
	    real32 inverse_aspect_ratio = (real32)buffer->width / buffer->height;
	    real32 real_scan_y = (buffer->height/2 + i / inverse_aspect_ratio);
	    game_state->floorcast_table[i] = ((real32)buffer->height /
						(2*real_scan_y - buffer->height));
	}
	
	memory->is_initialized = true;
    }

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

    int32 tile_size_in_pixels = 32;    
    real32 meters_to_pixels = (real32)tile_size_in_pixels;

    if (!input->keyboard.space)
    {
	Projection_Spec projection_spec = {};
	projection_spec.dim = 1.0f;
	projection_spec.fov = pi32 / 3.0f;
	projection_spec.view_distance = projection_spec.dim / 2.0f * sqrtf(3.0f);

	World_Spec world_spec = {};
	world_spec.wall_width = 1.0f;
	world_spec.wall_height = 1.0f;
	
	int32 ray_count = buffer->width;
	real32 left_most_angle = game_state->player_angle + projection_spec.fov/2.0f;
	real32 delta_angle = projection_spec.fov / (real32)ray_count;

	/*NOTE(chen):
	  The aspect ratio of wall dimension (world coordinate) must match the
	  inverse of that of screen dimension (screen coordinate)
	*/	    
	real32 inverse_aspect_ratio = (real32)buffer->width / (real32)buffer->height;
	
	for (int32 slice_index = 0; slice_index < ray_count; ++slice_index)
	{
	    //get reflection sample
	    real32 angle = left_most_angle - delta_angle*slice_index;
	    recanonicalize_angle(&angle);
	    Reflection_Sample reflection = cast_ray(&game_state->tile_map, game_state->player_position, angle);

	    //correct the ray length to perpendicular
	    real32 beta = angle - game_state->player_angle;
	    reflection.ray_length *= cosf(beta);

	    real32 projected_wall_height = (world_spec.wall_height / reflection.ray_length *
					    inverse_aspect_ratio);
	    
	    //calculate the upper and lower end of this slice of wall
	    int32 wall_slice_height = (int32)(projected_wall_height * buffer->height);
	    int32 wall_top = (int32)(buffer->height - wall_slice_height) / 2;

	    //rendering & texture mapping the walls
	    {
		Loaded_Image *wall_texture = &game_state->wall_texture;
	    
		real32 texture_x_unscaled = 0.0f;
		if (reflection.x_side_faced)
		{
		    real32 subtracter = floorf(reflection.hit_position.x);
		    texture_x_unscaled = reflection.hit_position.x - subtracter;
		}
		else
		{
		    real32 subtracter = floorf(reflection.hit_position.y);
		    texture_x_unscaled = reflection.hit_position.y - subtracter;
		}
		int32 texture_x = (int32)(texture_x_unscaled * (wall_texture->width - 1));
		copy_slice(buffer, wall_texture, texture_x, slice_index,
			   wall_top, wall_slice_height);

		//darken the slice if it's x faced
		if (!reflection.x_side_faced)
		{
		    uint32 *buffer_pixels = (uint32 *)buffer->memory;		    
		    for (int32 y = wall_top; y < wall_slice_height+wall_top; ++y)
		    {
			if (y < 0) y = 0;
			if (y >= buffer->height) break;

			buffer_pixels[slice_index + y*buffer->width] =
			    ((buffer_pixels[slice_index + y*buffer->width] >> 1) & 0x7F7F7F);
		    }
		}
	    }

	    //floor casting & texturing
	    {
		Loaded_Image *floor_texture = &game_state->floor_texture;
		Loaded_Image *ceiling_texture = &game_state->ceiling_texture;
		
		//scan downward from the bottom of the wall and locate & draw floor pixels 
		for (int32 scan_y = wall_top + wall_slice_height; scan_y < buffer->height; ++scan_y)
		{
		    /*NOTE(chen):
		      this scan_y is fake, because it was stretched by multiplying
		      inverse aspect ratio, in order to get the real scan y, we must
		      reverse the process and get the real_scan_y.

		      Note that, this process is raycasting, which means casting a ray from
		      projection onto the world to find the world coordinate, this scenario indicates
		      that in our renderer, Y coordinates are fucked due to the inverse
		      aspect ratio multiplier, so if we were to encounter a similiar situation,
		      we must take this into account.
		      
		      reverse the effect that inverse aspect ratio multiplier,
		      let this code calculate what it would be if the wall is not stretched,
		      then cast it back to our stretched coordinates
		    */

//TODO(chen): change this hard-coded renderer table to real code		    
#if 1
		    real32 current_dist = game_state->floorcast_table[scan_y - buffer->height/2];
#else
		    real32 real_scan_y = (buffer->height/2 +
					  (scan_y - buffer->height/2)/inverse_aspect_ratio);
		    real32 current_dist = ((real32)buffer->height /
					   (2*real_scan_y - buffer->height));
#endif
		    
		    real32 interpolent = (current_dist / reflection.ray_length);
		    v2 hit_position = reflection.hit_position;
		    v2 player_position = game_state->player_position;

/*NOTE(chen): for some reason, if code is compiled with -Od, inline functions are disabled,
              so for now, let's do it inlined manually. turn it back when compiled with -O2
 */
#if RELEASE_BUILD
		    v2 floor_position = lerp(player_position, hit_position, interpolent);
#else
		    v2 floor_position = {(player_position.x*(1.0f - interpolent) +
					  hit_position.x*interpolent),
					 (player_position.y*(1.0f - interpolent) +
					  hit_position.y*interpolent)};
#endif

		    int32 texture_x = ((int32)(floor_position.x*floor_texture->width) %
				       floor_texture->width);
		    int32 texture_y = ((int32)(floor_position.y*floor_texture->height) %
				       floor_texture->height);

		    int32 screen_x = slice_index;
		    int32 screen_y = scan_y;

		    uint32 *dest_pixels = (uint32 *)buffer->memory;
		    uint32 *floor_source_pixels = (uint32 *)floor_texture->data;
		    uint32 *ceiling_source_pixels = (uint32 *)ceiling_texture->data;
		    
		    dest_pixels[screen_x + screen_y*buffer->width] =
			floor_source_pixels[texture_x + texture_y*floor_texture->width];
		    
		    int32 reverse_scan_y = buffer->height - screen_y;
		    dest_pixels[screen_x + reverse_scan_y*buffer->width] =
			ceiling_source_pixels[texture_x + texture_y*floor_texture->width];
		}
	    }
	}
    }
    //NOTE(chen): Debug top-down view
    else
    {
	for (int32 y = 0; y < map_height; ++y)
	{
	    for (int32 x = 0; x < map_width; ++x)
	    {
		uint32 tile_value = get_tile_value(&game_state->tile_map, x, y);

		uint32 color = 0x000055;
		if (tile_value == 1)
		{
		    color = 0x0000FF;
		}

		int32 upper_left = (int32)tile_size_in_pixels*x;
		int32 upper_top = buffer->height - (int32)tile_size_in_pixels*(y+1);
		draw_rectangle(buffer, upper_left, upper_top,
			       upper_left + (int32)tile_size_in_pixels-1,
			       upper_top + (int32)tile_size_in_pixels-1, color);
	    }
	}

	int32 player_size_in_pixels = 8;
	int32 player_upper_left = (int32)(game_state->player_position.x*meters_to_pixels - player_size_in_pixels/2);
	int32 player_upper_top = (int32)(buffer->height - game_state->player_position.y*meters_to_pixels - player_size_in_pixels/2);
	int32 player_lower_right = player_upper_left + player_size_in_pixels;
	int32 player_lower_bottom = player_upper_top + player_size_in_pixels;
	draw_rectangle(buffer, player_upper_left, player_upper_top,
		       player_lower_right, player_lower_bottom, 0x00FFFF00);

	//test ray-casting
	int32 ray_count = 960;
	real32 fov = pi32 / 3.0f;
	real32 starting_angle = game_state->player_angle - fov/2.0f;
	real32 angle_step = fov / (real32)ray_count;
    
	for (int32 i = 0; i < ray_count; ++i)
	{
	    real32 angle = starting_angle + (angle_step*i);
	    recanonicalize_angle(&angle);
	    real32 ray_length = cast_ray(&game_state->tile_map, game_state->player_position, angle).ray_length;
	    real32 ray_end_x = game_state->player_position.x + cosf(angle) * ray_length;
	    real32 ray_end_y = game_state->player_position.y + sinf(angle) * ray_length;
						   
	    draw_line(buffer,
		      (int32)(game_state->player_position.x*meters_to_pixels),
		      (int32)(buffer->height-game_state->player_position.y*meters_to_pixels),
		      (int32)(ray_end_x*meters_to_pixels-1),
		      (int32)(buffer->height - ray_end_y*meters_to_pixels-1),
		      0x00FF5500);
	}
    }
	/*END OF DEBUG DRAWING*/
}
