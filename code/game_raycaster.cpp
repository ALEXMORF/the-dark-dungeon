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
		result.is_valid = true;
	    }
	}
    }

    assert(result.is_valid);
    return result;
}
