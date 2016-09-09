#include "game_sprite.h"

internal void
compute_sprite_order(Sprite *sprite_list, int32 len, v2 relative_position)
{
    for (int32 i = 0; i < len; ++i)
    {
	sprite_list[i].distance_squared = get_distance_squared(sprite_list[i].position, relative_position);
    }
}

internal void
add_sprite(Sprite_List *list, Sprite sprite)
{
    assert(list->count+1 < list->capacity);
    list->content[list->count++] = sprite;
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

inline int32
get_current_playing_index(real32 timer, real32 period, int32 index_count)
{
    int32 texture_index;  
    if (timer <= period)
    {
	texture_index = (int32)(timer / (period / index_count));
    }
    else
    {
	texture_index = index_count-1;
    }

    return texture_index;
}

internal Loaded_Image
get_currently_playing_texture(Game_State *game_state, Entity *entity)
{
    Loaded_Image result = {};

    switch (entity->type)
    {
	case barrel:
	{
	    result = game_state->barrel_texture;
	} break;

	case pillar:
	{
	    result = game_state->pillar_texture;
	} break;

	case guard:
	{
	    if (entity->hp == 0)
	    {
		real32 death_animation_period = 0.5f;
		int32 death_animation_index_count = 5;
		
		int32 texture_index_x = get_current_playing_index(entity->death_timer, death_animation_period, death_animation_index_count);  
		result = extract_image_from_sheet(&game_state->guard_texture_sheet, texture_index_x, 5);
	    }
	    else
	    {
		result = extract_image_from_sheet(&game_state->guard_texture_sheet, 0, 0);
	    }
	} break;

	case ss:
	{
	    if (entity->hp == 0)
	    {
		real32 death_animation_period = 0.5f;
		int32 death_animation_index_count = 5;
		    
		int32 texture_index_x = get_current_playing_index(entity->death_timer, death_animation_period, death_animation_index_count);  
		result = extract_image_from_sheet(&game_state->ss_texture_sheet, texture_index_x, 5);
	    }
	    else
	    {
		result = extract_image_from_sheet(&game_state->ss_texture_sheet, 0, 0);
	    }
		
	} break;
	    
	default:
	{
	    assert(!"unknown entity type");
	};
    }

    return result;
}

internal void
generate_sprite_list(Game_State *game_state, Sprite_List *list,
		     Entity *entity_list, int32 entity_count)
{
    //reset
    list->count = 0;
    
    for (int32 i = 0; i < entity_count; ++i)
    {
	bool32 is_dead = (entity_list[i].hp == 0);
	
	Sprite temp = {};
	temp.owner = &entity_list[i];
	temp.size.x = 1.0f;
	temp.size.y = 1.0f;
	temp.position = entity_list[i].position;
	temp.texture = get_currently_playing_texture(game_state, &entity_list[i]);
	add_sprite(list, temp);
    }

}

