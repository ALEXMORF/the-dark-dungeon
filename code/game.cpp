#include "game.h"

#include "game_math.cpp"
#include "game_tiles.cpp"

#include "game_asset.cpp"
#include "game_render.cpp"
#include "game_raycaster.cpp"

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    assert(sizeof(Game_State) <= memory->permanent_storage_size);
    Game_State *game_state = (Game_State *)memory->permanent_storage;
    
    if (!memory->is_initialized)
    {
	game_state->player_position = {3.0f, 3.0f};
	game_state->player_angle = 0.0f;
	
	game_state->wall_texture = load_image(memory->platform_load_image, "../data/greystone.png");
	assert(game_state->wall_texture.data);
	       
	memory->is_initialized = true;
    }

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
    Tile_Map tile_map = {};
    tile_map.tiles = tiles;
    tile_map.exception_tile_value = 1;
    tile_map.tile_count_x = map_width;
    tile_map.tile_count_y = map_height;

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

	int32 ray_count = buffer->width;
	real32 left_most_angle = game_state->player_angle + projection_spec.fov/2.0f;
	real32 delta_angle = projection_spec.fov / (real32)ray_count;

	for (int32 slice_index = 0; slice_index < ray_count; ++slice_index)
	{
	    //get reflection sample
	    real32 angle = left_most_angle - delta_angle*slice_index;
	    recanonicalize_angle(&angle);
	    Reflection_Sample reflection = cast_ray(&tile_map,
						    game_state->player_position,
						    angle);
	    
	    //adjust sample's ray length
	    real32 beta = angle - game_state->player_angle;
	    reflection.ray_length *= cosf(beta);
	    
	    //calculate projection
	    /*NOTE(chen): yo check this shit out!!!
	      I already set the projection width to 1.0f in world coord,
	      and set the wall width to the same as wall height,
	      however, the screen is wider than its height,
	      so therefore, when a wall is up close in your face,
	      the wall-width fills the projection-width,
	      however, wall-height can't just be 1.0f, which is equal to projection height,
	      because that would mean it's attached to the aspect ratio of screen.
	      To acheive realistic effect,
	      it must exceed to to simulate the effect of a cube of equal sides

	      TL:DR,
	      The aspect ratio of wall dimension (world coordinate) must match the
	      inverse of that of screen dimension (screen coordinate)
	     */
	    real32 wall_height = projection_spec.dim * (real32)buffer->width / (real32)buffer->height;
	    real32 projected_wall_height = (real32)(wall_height / reflection.ray_length);
	    
	    //draw the actual slice
	    int32 wall_slice_height = (int32)(projected_wall_height * buffer->height);
	    int32 wall_top = (int32)(buffer->height - wall_slice_height) / 2;

	    //texture mapping & rendering
	    {
		Loaded_Image *wall_texture = &game_state->wall_texture;
	    
		real32 texture_x_unscaled = 0.0f;
		if (reflection.x_side_faced)
		{
		    real32 divisor = floorf(reflection.hit_position.x);
		    texture_x_unscaled = reflection.hit_position.x - divisor;
		}
		else
		{
		    real32 divisor = floorf(reflection.hit_position.y);
		    texture_x_unscaled = reflection.hit_position.y - divisor;
		}
		int32 texture_x = (int32)(texture_x_unscaled * (wall_texture->width - 1));

		copy_slice(buffer, wall_texture, texture_x, slice_index, wall_top, wall_slice_height);
	    }
	}
    }
    else
    {
	for (int32 y = 0; y < map_height; ++y)
	{
	    for (int32 x = 0; x < map_width; ++x)
	    {
		uint32 tile_value = get_tile_value(&tile_map, x, y);

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
	    real32 ray_length = cast_ray(&tile_map, game_state->player_position, angle).ray_length;
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
