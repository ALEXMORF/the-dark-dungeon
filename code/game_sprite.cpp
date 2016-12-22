#include "game_sprite.h"

internal void
compute_sprite_order(Sprite *sprite_list, int32 len, v2 relative_position)
{
    for (int32 i = 0; i < len; ++i)
    {
        sprite_list[i].distance_squared = len_squared(sprite_list[i].position - relative_position);
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

inline int32
get_directional_index(int32 zero_dir_index, int32 dir_index_count, real32 entity_angle, real32 player_angle)
{
    real32 angle_diff_each_index = pi32*2.0f / (real32)dir_index_count;
    real32 angle_diff = get_angle_diff(entity_angle, player_angle);
    recanonicalize_angle(&angle_diff);
    angle_diff += angle_diff_each_index/2.0f;
    int32 dir_index = (zero_dir_index + (int32)(angle_diff / angle_diff_each_index)) % dir_index_count;

    return dir_index;
}

inline int32
get_walk_index(int32 init_stride, int32 walk_index_count, real32 walk_cycle_period, real32 walk_timer)
{
    real32 walk_index_interval = walk_cycle_period / (real32)walk_index_count;
    int32 walk_index = (int32)(walk_timer / walk_index_interval) % walk_index_count;

    return (walk_index + init_stride);
}

internal Loaded_Image
get_currently_playing_texture(Game_Asset *game_asset, Entity *entity, real32 camera_angle)
{
    Loaded_Image result = {};
    
    switch (entity->type)
    {
        case ENTITY_TYPE_GUARD:
        {
            if (entity->state == death_state)
            {
                real32 death_animation_period = 0.5f;
                int32 death_animation_index_count = 5;
                
                int32 texture_index_x = get_current_playing_index(entity->clock[death_state],
                                                                  death_animation_period,
                                                                  death_animation_index_count);  
                result = extract_image_from_sheet(&game_asset->guard_texture_sheet, texture_index_x, 5);
            }
            else if (entity->state == hurting_state)
            {
                result = extract_image_from_sheet(&game_asset->guard_texture_sheet, 7, 5);
            }
            else if (entity->state == aiming_state)
            {
                real32 prepare_period = 0.3f;

                //get variant
                Aiming_State *aiming_state = (Aiming_State *)entity->variant_block.storage;

                if (aiming_state->firing_animation_cd > 0)
                {
                    result = extract_image_from_sheet(&game_asset->guard_texture_sheet, 2, 6);
                }
                else
                {
                    int32 x_index = 1;
                    if (entity->clock[entity->state] < prepare_period)
                    {
                        x_index = 0;
                    }
                    result = extract_image_from_sheet(&game_asset->guard_texture_sheet, x_index, 6);
                }
            }
            else
            {
                real32 walk_cycle_period = 1.0f;
                int32 walk_index_count = 4;
                int32 walk_init_stride = 1;
                
                int32 dir_index = get_directional_index(4, 8, entity->angle, camera_angle);
                int32 walk_index = get_walk_index(walk_init_stride, walk_index_count, walk_cycle_period, entity->clock[walking_state]);
                if (entity->state != walking_state)
                {
                    walk_index = 0;
                }
                
                result = extract_image_from_sheet(&game_asset->guard_texture_sheet, dir_index, walk_index);
            }
            
        } break;
        
        case ENTITY_TYPE_SS:
        {
            if (entity->state == death_state)
            {
                real32 death_animation_period = 0.5f;
                int32 death_animation_index_count = 5;
                
                int32 texture_index_x = get_current_playing_index(entity->clock[death_state], death_animation_period, death_animation_index_count);  
                result = extract_image_from_sheet(&game_asset->ss_texture_sheet, texture_index_x, 5);
            }
            else if (entity->state == hurting_state)
            {
                result = extract_image_from_sheet(&game_asset->ss_texture_sheet, 7, 5);         
            }
            else if (entity->state == aiming_state)
            {
                real32 prepare_period = 0.2f;
                
                Aiming_State *aiming_state = (Aiming_State *)entity->variant_block.storage;

                if (aiming_state->firing_animation_cd > 0)
                {
                    result = extract_image_from_sheet(&game_asset->ss_texture_sheet, 2, 6);
                }
                else
                {
                    int32 x_index = 1;
                    if (entity->clock[entity->state] < prepare_period)
                    {
                        x_index = 0;
                    }
                    result = extract_image_from_sheet(&game_asset->ss_texture_sheet, x_index, 6);
                }
            }
            else 
            {
                real32 walk_cycle_period = 1.0f;
                int32 walk_index_count = 4;
                int32 walk_init_stride = 1;
                
                int32 dir_index = get_directional_index(4, 8, entity->angle, camera_angle);
                int32 walk_index = get_walk_index(walk_init_stride, walk_index_count, walk_cycle_period, entity->clock[walking_state]);
                if (entity->state != walking_state)
                {
                    walk_index = 0;
                }
                
                result = extract_image_from_sheet(&game_asset->ss_texture_sheet, dir_index, walk_index);
            }
            
        } break;
            
        default:
        {
            result = entity->sprite;
        };
    }
    
    return result;
}

internal void
generate_sprite_list(Game_Asset *game_asset, Sprite_List *list,
                     DBuffer(Entity) *entity_buffer, real32 camera_angle)
{
    //reset
    list->count = 0;
    
    for (int32 i = 0; i < entity_buffer->count; ++i)
    {
        Sprite temp = {};
        temp.owner = &entity_buffer->e[i];
        temp.size.x = 1.0f;
        temp.size.y = 1.0f;
        temp.position = entity_buffer->e[i].body.position;
        temp.texture = get_currently_playing_texture(game_asset, &entity_buffer->e[i], camera_angle);
        add_sprite(list, temp);
    }

}

