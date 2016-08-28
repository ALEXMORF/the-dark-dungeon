#include "game.h"

#define array_count(array) (sizeof(array)/sizeof(array[0]))

#include "game_math.cpp"
#include "game_tiles.cpp"

#include "game_asset.cpp"
#include "game_render.cpp"
#include "game_raycaster.cpp"

//TODO(chen): slow, replace with intrinsics 
inline void
zero_memory(void *ptr, uint32 size)
{
    uint32 chunk_count = size / 8; //8-byte chunks
    uint32 left_over = size % 8;

    uint64 *writer = (uint64 *)ptr;
    for (uint32 i = 0; i < chunk_count; ++i)
    {
	*writer++ = 0;
    }

    uint8 *small_writer = (uint8 *)writer;
    for (uint32 i = 0; i < left_over; ++i)
    {
	*small_writer++ = 0;
    }
}

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
	//memory
	initialize_linear_allocator(&game_state->permanent_allocator,
				    (uint8 *)memory->permanent_storage + sizeof(Game_State),
				    memory->permanent_storage_size - sizeof(Game_State));
	initialize_linear_allocator(&game_state->transient_allocator,
				    (uint8 *)memory->transient_storage,
				    memory->transient_storage_size);
	
	//game
//TODO(chen): this is some improv tile-map init code, replace this with procedural generation later
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
	tile_map->tiles = Push_Array(&game_state->permanent_allocator, tile_count, uint32);
	Copy_Array(tiles, tile_map->tiles, tile_count, uint32);

	game_state->player_position = {3.0f, 3.0f};
	game_state->player_angle = 0.0f;
	game_state->barrel_position = {5.0f, 5.0f};
	    
	//asset
	game_state->wall_texture = load_image(memory->platform_load_image,
					      "../data/redbrick.png");
	game_state->floor_texture = load_image(memory->platform_load_image,
					       "../data/greystone.png");
	game_state->ceiling_texture = load_image(memory->platform_load_image,
						 "../data/greystone.png");
	game_state->barrel_texture = load_image(memory->platform_load_image,
						"../data/barrel.png");
	assert(game_state->wall_texture.data);
	assert(game_state->floor_texture.data);
	assert(game_state->ceiling_texture.data);
	assert(game_state->barrel_texture.data);

	//floorcast lookup table
	game_state->floorcast_table_count = buffer->height/2;
	game_state->floorcast_table = Push_Array(&game_state->permanent_allocator,
						 game_state->floorcast_table_count, real32);	
	for (int32 i = 0; i < game_state->floorcast_table_count; ++i)
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
	    real32 inverse_aspect_ratio = (real32)buffer->width / buffer->height;
	    real32 real_scan_y = (buffer->height/2 + i / inverse_aspect_ratio);
	    game_state->floorcast_table[i] = ((real32)buffer->height /
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

	real32 *z_buffer = Push_Array(&game_state->transient_allocator, buffer->width, real32);
	
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

	    //NOTE(chen): fix fisheye
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

		Pixel_Manip *pixel_manipulator = 0;
		if (!reflection.x_side_faced)
		{
		    pixel_manipulator = darken;
		}
		
		copy_slice(buffer, wall_texture, texture_x, slice_index,
			   wall_top, wall_slice_height, pixel_manipulator);
	    }

	    //floor casting & texturing
	    {
		Loaded_Image *floor_texture = &game_state->floor_texture;
		Loaded_Image *ceiling_texture = &game_state->ceiling_texture;
		
		//scan downward from the bottom of the wall and locate & draw floor pixels 
		for (int32 scan_y = wall_top + wall_slice_height; scan_y < buffer->height; ++scan_y)
		{

		    //NOTE(chen): uses a look-up table to find distance, fast
		    real32 current_dist = game_state->floorcast_table[scan_y - buffer->height/2];
		    
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
	    
	    //record the ray-length in a z-buffer
	    z_buffer[slice_index] = reflection.ray_length;
	} 

	//TODO(chen): render sprites
	{
	    v2 player_to_sprite = game_state->barrel_position - game_state->player_position;
	    real32 direction_angle = atan2f(player_to_sprite.y, player_to_sprite.x);
	    
	    real32 player_to_sprite_distance = sqrtf(player_to_sprite.x*player_to_sprite.x +
						     player_to_sprite.y*player_to_sprite.y);
	    //NOTE(chen): fix fisheye
	    player_to_sprite_distance *= cosf(direction_angle - game_state->player_angle);
	    


	    //calculate sprite GroundPoint
	    v2 sprite_ground_point = {};
	    sprite_ground_point.x = (left_most_angle - direction_angle) / delta_angle;
	    for (int32 table_index = 0;
		 table_index < game_state->floorcast_table_count-1;
		 ++table_index)
	    {
		if (game_state->floorcast_table[table_index] > player_to_sprite_distance &&
		    game_state->floorcast_table[table_index+1] < player_to_sprite_distance)
		{
		    sprite_ground_point.y = buffer->height/2 + (real32)table_index;
		}
	    }

	    int32 sprite_height = 80;
	    int32 sprite_width = 20;
	    int32 sprite_upper_left = (int32)(sprite_ground_point.x - sprite_width/2);
	    int32 sprite_upper_top = (int32)(sprite_ground_point.y - sprite_height);
	    int32 sprite_lower_right = (int32)(sprite_ground_point.x + sprite_width/2);
	    int32 sprite_lower_bottom = (int32)(sprite_ground_point.y);
	    
	    draw_rectangle(buffer, sprite_upper_left, sprite_upper_top,
			   sprite_lower_right, sprite_lower_bottom,
			   0);
	}
    }
    //NOTE(chen): Debug top-down view
    else
    {
	int32 tile_size_in_pixels = 32;    
	real32 meters_to_pixels = (real32)tile_size_in_pixels;

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
