#include "game_entity.h"

internal bool32
player_handle_input(Player *player, Game_Input *input, Platform_Play_Sound *play_sound)
{
    bool32 player_shot = false;
    
    real32 player_speed = 2.5f;
    real32 lerp_constant = 0.4f;
    real32 mouse_sensitivity = 0.7f;
    char *gun_sound_file_name = "../data/pistol.wav";
    
    real32 forward = 0.0f;
    real32 left = 0.0f;
    if_do(input->keyboard.left, left = 1.0f);
    if_do(input->keyboard.right, left = -1.0f);
    if_do(input->keyboard.up, forward = 1.0f);
    if_do(input->keyboard.down, forward = -1.0f);
    
    v2 player_d_velocity = {};
    player_d_velocity.x += cosf(player->angle) *forward;    
    player_d_velocity.y += sinf(player->angle) * forward;
    player_d_velocity.x += cosf(player->angle + pi32/2.0f) * left;    
    player_d_velocity.y += sinf(player->angle + pi32/2.0f) * left;
    player_d_velocity = normalize(player_d_velocity);
    
    player_d_velocity *= player_speed * input->dt_per_frame;
    player->velocity = lerp(player->velocity, player_d_velocity, lerp_constant);
    
    if (input->mouse.down && player->weapon_cd_counter == 0.0f)
    {
	play_sound(gun_sound_file_name);	    
	player->weapon_cd_counter = player->weapon_cd;

	player_shot = true;
    }

    real32 player_delta_angle = -input->mouse.dx / 500.0f * pi32/3.0f * mouse_sensitivity;
    player->angle += player_delta_angle;
    recanonicalize_angle(&player->angle);

    return player_shot;
}

internal void
player_update(Player *player, real32 dt)
{
    real32 animation_cycle = 0.78f;
    real32 animation_index_count = 4.0f;
    int32 animation_ending_index = 1;
    
    player->position += player->velocity;

    if (player->weapon_cd_counter)
    {
	real32 time_passed = player->weapon_cd - player->weapon_cd_counter;
	if (time_passed < animation_cycle)
	{
	    real32 animation_index_interval = animation_cycle / animation_index_count;
	    player->weapon_animation_index = (int32)(time_passed / animation_index_interval + (animation_ending_index + 1)) % ((int32)animation_index_count + 1);
	}
	else
	{
	    player->weapon_animation_index = animation_ending_index;
	}
	
	player->weapon_cd_counter -= (dt < player->weapon_cd_counter? dt: player->weapon_cd_counter);
    }
}

internal void
add_entity(Entity_List *list, Entity entity)
{
    assert(list->count+1 < ENTITY_COUNT_MAX);
    list->content[list->count++] = entity;
}

inline Entity
make_guard(v2 position, int32 hp = 1)
{
    Entity result = {};
    result.type = guard;
    result.position = position;
    result.angle = 0.0f;
    result.hp = hp;
    
    return result;
}
