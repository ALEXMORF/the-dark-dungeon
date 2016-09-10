#include "game_simulate.h"

internal void
player_handle_input(Player *player, Game_Input *input)
{
    real32 player_speed = 2.5f;
    real32 lerp_constant = 0.2f;
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
	player->weapon_cd_counter = player->weapon_cd;

	player->has_fired = true;
    }
    else
    {
	player->has_fired = false;
    }
    
    real32 player_delta_angle = -input->mouse.dx / 500.0f * pi32/3.0f * mouse_sensitivity;
    player->angle += player_delta_angle;
    recanonicalize_angle(&player->angle);
}

internal void
player_update(Player *player, real32 dt)
{
    if (player->weapon_cd_counter)
    {
	player->weapon_cd_counter -= (dt < player->weapon_cd_counter? dt: player->weapon_cd_counter);
    }
    
    player->position += player->velocity;
}

internal void
simulate_world(Game_State *game_state, Game_Input *input)
{
    Player *player = &game_state->player;
    
    player_handle_input(player, input);
    
    if (player->has_fired) 
    {
	game_state->need_to_play_pistol_sound = true;
	
	if (game_state->currently_aimed_entity != 0 && game_state->currently_aimed_entity->hp != 0)
	{
	    --game_state->currently_aimed_entity->hp;
	}
    }
    
    player_update(player, input->dt_per_frame);
    
    for (int32 i = 0; i < game_state->entity_list.count; ++i)
    {
	Entity *entity = &game_state->entity_list.content[i];
	
	if (entity->hp == 0)
	{
	    entity->death_timer += input->dt_per_frame;
	}
    }
}
