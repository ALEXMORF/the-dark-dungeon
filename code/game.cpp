#include "game.h"

#define abs(value) ((value) > 0? (value): -(value))
#define swap(x, y) do {x ^= y; y ^= x; x ^= y;} while (0)

#define assert(value) do {if(!(value)) *(int*)0 = 0;} while (0)
#define if_do(condition, action) do { if (condition) action; } while (0)

#include "game_math.cpp"

internal void
fill_buffer(Game_Offscreen_Buffer *buffer, uint32 color)
{
    uint32 *pixel = (uint32 *)buffer->memory;
    int32 pixel_count = buffer->width * buffer->height;
    for (int i = 0; i < pixel_count; ++i)
    {
	pixel[i] = color;
    }
}

internal void
draw_line(Game_Offscreen_Buffer *buffer, int32 x0, int32 y0, int32 x1, int32 y1, uint32 color)
{
    uint32 *pixels = (uint32 *)buffer->memory;
    
    real32 slope = (real32)(y1 - y0)/(x1 - x0);
    if (abs(slope) > 1.0f)
    {
	if (y0 > y1)
	{
	    swap(x0, x1);
	    swap(y0, y1);
	}
	slope = 1.0f / slope;
	for (int32 y = y0; y <= y1; ++y)
	{
	    int32 x = (int32)(x0 + slope*(y - y0));
	    if (x >= 0 && x < buffer->width && y >= 0 && y < buffer->height)
	    {
		pixels[buffer->width * y + x] = color;
	    }
	}
    }
    else
    {
	if (x0 > x1)
	{
	    swap(x0, x1);
	    swap(y0, y1);
	}
	for (int32 x = x0; x <= x1; ++x)
	{
	    int32 y = (int32)(y0 + slope*(x - x0));
	    if (x >= 0 && x < buffer->width && y >= 0 && y < buffer->height) 
	    {
		pixels[buffer->width * y + x] = color;
	    }
	}
    }
}

internal void
draw_rectangle(Game_Offscreen_Buffer *buffer,
	       int32 min_x, int32 min_y,
	       int32 max_x, int32 max_y,
	       uint32 color)
{
    if (min_x < 0)
    {
	min_x = 0;
    }
    if (min_y < 0)
    {
	min_y = 0;
    }
    if (max_x > buffer->width)
    {
	max_x = buffer->width;
    }
    if (max_y > buffer->height)
    {
	max_y = buffer->height;
    }

    int32 bytes_per_pixel = 4;
    
    uint8 *row = (uint8 *)buffer->memory;
    row += bytes_per_pixel*min_x + buffer->pitch*min_y;

    for (int32 y = min_y; y < max_y; ++y)
    {
	uint32 *pixel = (uint32 *)row;
	for (int32 x = min_x; x < max_x; ++x)
	{
	    *pixel++ = color;
	}
	row += buffer->pitch;
    }
}

inline uint32
get_tile_value(Tile_Map *tile_map, int32 x, int32 y)
{
    uint32 result = tile_map->exception_tile_value;
    if (x >= 0 && x < tile_map->tile_count_x && y >= 0 && y < tile_map->tile_count_y)
    {
	result = tile_map->tiles[x + (tile_map->tile_count_y-y-1)*tile_map->tile_count_x];
    }
    
    return result;
}

inline void
recanonicalize_angle(real32 *angle)
{
    while (*angle >= pi32*2.0f)
    {
	*angle -= pi32*2.0f;
    }
    while (*angle < 0.0f)
    {
	*angle += pi32*2.0f;
    }
}

internal Reflection_Sample
cast_ray(Tile_Map *tile_map, v2 position, real32 angle)
{
    Reflection_Sample result = {};
    
    real32 invalid_value = -1.0f;    
    //NOTE(chen): the return value of this function call
    result.ray_length = invalid_value;
    
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
	    result.x_side_faced = true;
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
	    if (result.ray_length == invalid_value || result.ray_length > temp)
	    {
		result.ray_length = temp;
		result.x_side_faced = false;
	    }
	}
    }

    assert(result.ray_length != invalid_value);
    return result;
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    assert(sizeof(Game_State) <= memory->permanent_storage_size);
    Game_State *game_state = (Game_State *)memory->permanent_storage;
    
    if (!memory->is_initialized)
    {
	game_state->player_position.x = 3.0f;
	game_state->player_position.y = 3.0f;
	game_state->player_angle = 0.0f;
	
	memory->is_initialized = true;
    }

#define map_width 10
#define map_height 10
    uint32 tiles[map_width*map_height] =
	{
	    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	    1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	    1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
	    1, 0, 0, 0, 0, 0, 1, 1, 1, 1,
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

    v2 player_displacement = {};
    if (input->keyboard.left)
    {
	player_displacement.x = -1.0f;
    }
    if (input->keyboard.right)
    {
	player_displacement.x = 1.0f;
    }
    if (input->keyboard.up)
    {
	player_displacement.y = 1.0f;
    }
    if (input->keyboard.down)
    {
	player_displacement.y = -1.0f;
    }
    game_state->player_position += player_displacement * input->dt_per_frame;
    
    //
    //
    //
    
#if 0
    real32 player_delta_angle = degree_to_radian(30.0f);
    game_state->player_angle += player_delta_angle*input->dt_per_frame;
    recanonicalize_angle(&game_state->player_angle);
#endif
    
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

	for (int32 i = 0; i < ray_count; ++i)
	{
	    //calculate ray length
	    real32 angle = left_most_angle - delta_angle*i;
	    recanonicalize_angle(&angle);
	    Reflection_Sample reflection = cast_ray(&tile_map,
						    game_state->player_position,
						    angle);
	    
	    //adjust ray length
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
	    
	    real32 projected_wall_height = (wall_height * projection_spec.view_distance /
					    (reflection.ray_length + projection_spec.view_distance));
	    int32 wall_slice_height = (int32)(projected_wall_height * buffer->height);
	    int32 wall_top = (int32)(buffer->height - wall_slice_height) / 2;

	    //draw that slice to the screen
	    uint32 wall_color = 0x00555555;
	    if (reflection.x_side_faced)
	    {
		wall_color = 0x00FFFFFF;		
	    }
	    draw_line(buffer, i, wall_top, i, wall_top+wall_slice_height, wall_color);
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
