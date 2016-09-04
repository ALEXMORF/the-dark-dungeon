#include "game_raycaster.h"

internal Reflection_Sample
cast_ray(Tile_Map *tile_map, v2 position, real32 angle)
{
    Reflection_Sample result = {};
    
    result.ray_length = -1.0f;
    
    int32 current_tile_x = (int32)position.x;
    int32 current_tile_y = (int32)position.y;
    
    bool32 angle_is_in_quadrant_1 = (angle > 0 && angle < pi32/2.0f);
    bool32 angle_is_in_quadrant_2 = (angle > pi32/2.0f && angle < pi32);
    bool32 angle_is_in_quadrant_3 = (angle > pi32 && angle < pi32*3.0f/2.0f);
    bool32 angle_is_in_quadrant_4 = (angle > pi32*3.0f/2.0f);
    
    //horizontal hit
    {
	//NOTE(chen): direction of the ray we are casting (right = 1.0f, left = -1.0f)
	real32 sign = 0.0f;

	if_do(angle_is_in_quadrant_4 || angle_is_in_quadrant_1 || angle == 0.0f, sign = 1.0f);
	if_do(angle_is_in_quadrant_2 || angle_is_in_quadrant_3 || angle == pi32, sign = -1.0f);

	if (sign != 0.0f)
	{
	    real32 initial_dx = (real32)(current_tile_x + (int32)(sign < 0.0f? 0: 1)) - position.x;
	    real32 initial_hit_x = position.x + initial_dx;
	    real32 initial_hit_y = position.y + tanf(angle)*initial_dx;
	    
	    real32 hit_x = initial_hit_x;
	    real32 hit_y = initial_hit_y;

	    int32 tile_x_offset = (sign == -1.0f? -1: 0);	    
	    while (get_tile_value(tile_map, (int32)hit_x + tile_x_offset, (int32)hit_y) == 0)
	    {
		hit_x += sign;
		hit_y += tanf(angle)*sign;
	    }
	    
	    real32 dx = abs(hit_x - position.x);
	    real32 dy = abs(hit_y - position.y);
	    
	    result.ray_length = sqrtf(dx*dx + dy*dy);
	    result.x_side_faced = false;
	    result.hit_position = {hit_x, hit_y};
	    result.tile_x = (int32)hit_x + tile_x_offset;
	    result.tile_y = (int32)hit_y;
	    result.is_valid = true;
	}
    }
    
    //vertical hit
    {
	//NOTE(chen): direction of the ray we are casting (right = 1.0f, left = -1.0f)	
	real32 sign = 0.0f;

	if_do(angle_is_in_quadrant_1 || angle_is_in_quadrant_2 || angle == pi32/2.0f, sign = 1.0f);
	if_do(angle_is_in_quadrant_3 || angle_is_in_quadrant_4 || angle == pi32*3.0f/2.0f, sign = -1.0f);
	
	if (sign != 0.0f)
	{
	    real32 initial_dy = (real32)(current_tile_y + (int32)(sign < 0.0f? 0: 1)) - position.y;
	    real32 initial_hit_y = position.y + initial_dy;
	    real32 initial_hit_x = position.x + (initial_dy / tanf(angle));
	    
	    real32 hit_x = initial_hit_x;
	    real32 hit_y = initial_hit_y;

	    int32 tile_y_offset = (sign == -1.0f? -1: 0);
	    while (get_tile_value(tile_map, (int32)hit_x, (int32)hit_y + tile_y_offset) == 0)
	    {
		hit_x += sign/tanf(angle);
		hit_y += sign;
	    }
	    
	    real32 dx = abs(hit_x - position.x);
	    real32 dy = abs(hit_y - position.y);
	    real32 temp = sqrtf(dx*dx + dy*dy);
	    if (!result.is_valid || result.ray_length > temp)
	    {
		result.ray_length = temp;
		result.x_side_faced = true;
		result.hit_position = {hit_x, hit_y};
		result.tile_x = (int32)hit_x;
		result.tile_y = (int32)hit_y + tile_y_offset;
		result.is_valid = true;
	    }
	}
    }

    assert(result.is_valid);
    return result;
}

//NOTE(chen): precondition: have the sprite list sorted 
internal void
render_3d_scene(Game_Offscreen_Buffer *buffer, Render_Context *render_context,
		Tile_Map *tile_map, v2 position, real32 view_angle,
		Loaded_Image *ceiling_texture, Loaded_Image *floor_texture,
		Wall_Textures *wall_textures,
		Sprite *sprites, int32 sprite_count)
{
    real32 inverse_aspect_ratio = (real32)buffer->width / (real32)buffer->height;

    Projection_Spec projection_spec = {};
    projection_spec.dim = 1.0f;
    projection_spec.fov = pi32 / 3.0f;
    projection_spec.view_distance = projection_spec.dim / 2.0f * sqrtf(3.0f);

    World_Spec world_spec = {};
    world_spec.wall_width = 1.0f;
    world_spec.wall_height = 1.0f;
	
    int32 ray_count = buffer->width;
    real32 left_most_angle = view_angle + projection_spec.fov/2.0f;
    real32 delta_angle = projection_spec.fov / (real32)ray_count;
	
    for (int32 slice_index = 0; slice_index < ray_count; ++slice_index)
    {
	real32 angle = left_most_angle - delta_angle*slice_index;
	recanonicalize_angle(&angle);
	Reflection_Sample reflection = cast_ray(tile_map, position, angle);

	//NOTE(chen): fix fisheye
	reflection.ray_length *= cosf(angle - view_angle);

	//this is the z-buffer for sprite-rendering
	render_context->z_buffer[slice_index] = reflection.ray_length;
	    
	real32 projected_wall_height = (world_spec.wall_height / reflection.ray_length *
					inverse_aspect_ratio);
	int32 wall_slice_height = (int32)(projected_wall_height * buffer->height);
	int32 wall_top = (int32)(buffer->height - wall_slice_height) / 2;

	//wall rendering routine
	{
	    int32 tile_value = get_tile_value(tile_map, reflection.tile_x, reflection.tile_y);
	    assert(tile_value >= 1 && tile_value < wall_textures->count);
	    Loaded_Image *wall_texture = &wall_textures->E[tile_value-1];

	    real32 tile_size = 1.0f;
	    real32 texture_x_percentage = (reflection.x_side_faced?
					   modff(reflection.hit_position.x, &tile_size):
					   modff(reflection.hit_position.y, &tile_size));
	    int32 texture_x = (int32)(texture_x_percentage * (wall_texture->width - 1));

	    Shader_Fn *shader = (reflection.x_side_faced? 0: darken);
	    copy_slice(buffer, wall_texture, texture_x, slice_index,
		       wall_top, wall_slice_height, shader);
	}
	    
	//floor casting routine
	{
	    for (int32 scan_y = wall_top + wall_slice_height; scan_y < buffer->height; ++scan_y)
	    {

		real32 current_dist = render_context->floorcast_table[scan_y - buffer->height/2];
		    
		real32 interpolent = (current_dist / reflection.ray_length);
		v2 hit_position = reflection.hit_position;
		v2 player_position = position;
		    
#if RELAESE_BUILD
		v2 floor_position = lerp(player_position, hit_position, interpolent);
#else
		v2 floor_position = {};
		floor_position.x = (player_position.x * (1.0f - interpolent) +
				    hit_position.x * interpolent);
		floor_position.y =  (player_position.y * (1.0f - interpolent) +
				     hit_position.y * interpolent);
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
		dest_pixels[screen_x + (buffer->height - screen_y)*buffer->width] =
		    ceiling_source_pixels[texture_x + texture_y*floor_texture->width];
	    }
	} 
    } 

    for (int32 i = 0; i < sprite_count; ++i)
    //sprite rendering routine polishment
    {
	Loaded_Image *sprite_image = sprites[i].texture;
	real32 world_sprite_width = 1.0f;
	real32 world_sprite_height = 1.0f;

	v2 player_to_sprite = sprites[i].position - position;
	real32 direction_angle = atan2f(player_to_sprite.y, player_to_sprite.x);
	real32 beta = get_angle_diff(direction_angle, view_angle);
	    
	real32 player_to_sprite_distance = sqrtf(player_to_sprite.x*player_to_sprite.x +
						 player_to_sprite.y*player_to_sprite.y);

	//clip all sprites that are outside FOV
	if (abs(beta) < pi32 / 3.0f)
	{
	    //NOTE(chen): fix fisheye		
	    player_to_sprite_distance *= cosf(beta);

	    v2 sprite_ground_point = {};
	    {
		sprite_ground_point.x = (beta + projection_spec.fov/2.0f) / delta_angle;
		
		real32 d = player_to_sprite_distance;
		real32 scan_y = (0.5f - (d - 1.0f) / (2.0f * d)) * (real32)buffer->height;
		sprite_ground_point.y = scan_y*inverse_aspect_ratio + buffer->height/2;
	    }

	    real32 projection_scale = 1.0f / player_to_sprite_distance;
	    
	    int32 sprite_height = (int32)(projection_scale * world_sprite_height *
					  inverse_aspect_ratio * buffer->height);
	    int32 sprite_width = (int32)(projection_scale * world_sprite_width * buffer->width);
	    
	    int32 sprite_upper_left = (int32)(sprite_ground_point.x - sprite_width/2);
	    int32 sprite_upper_top = (int32)(sprite_ground_point.y - sprite_height);
	    int32 sprite_lower_right = (int32)(sprite_ground_point.x + sprite_width/2);
	    int32 sprite_lower_bottom = (int32)(sprite_ground_point.y);
	    
	    int32 sprite_texture_width = sprites[i].texture->width;
	    real32 texture_mapper = (real32)sprite_texture_width / sprite_width;
	    
	    real32 sprite_texture_x = 0.0f;
	    for (int32 slice_index = sprite_upper_left;
		 slice_index < sprite_lower_right;
		 ++slice_index)
	    {
		if (slice_index >= 0 && slice_index < buffer->width &&
		    player_to_sprite_distance < render_context->z_buffer[slice_index])
		{
		    copy_slice(buffer, sprite_image, (int32)(sprite_texture_x),
			       slice_index, sprite_upper_top, sprite_height, 0);
		}
		sprite_texture_x += texture_mapper;
	    }
	}
    }
}

internal void
compute_sprite_order(Sprite *sprite_list, int32 len, v2 relative_position)
{
    for (int32 i = 0; i < len; ++i)
    {
	sprite_list[i].distance_squared = get_distance_squared(sprite_list[i].position, relative_position);
    }
}

internal void
sort_sprites(Sprite *sprite_list, int32 len, v2 relative_position)
{
    compute_sprite_order(sprite_list, len, relative_position);
    
    for (int32 i = len-1; i > 0; --i)
    {
	for (int32 j = len - 1; j > 0; --j)
	{
	    if (sprite_list[j-1].distance_squared < sprite_list[j].distance_squared)
	    {
		Sprite temp = sprite_list[j-1];
		sprite_list[j-1] = sprite_list[j];
		sprite_list[j] = temp;
	    }
	}
    }
}
