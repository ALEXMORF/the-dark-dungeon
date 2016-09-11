#include "game_simulate.h"

void Player::input_process(Game_Input *input)
{
    real32 player_speed = 2.5f;
    real32 lerp_constant = 0.2f;
    real32 mouse_sensitivity = 0.7f;

    real32 forward = 0.0f;
    real32 left = 0.0f;
    if_do(input->keyboard.left, left = 1.0f);
    if_do(input->keyboard.right, left = -1.0f);
    if_do(input->keyboard.up, forward = 1.0f);
    if_do(input->keyboard.down, forward = -1.0f);
    
    v2 player_d_velocity = {};
    player_d_velocity.x += cosf(this->angle) *forward;    
    player_d_velocity.y += sinf(this->angle) * forward;
    player_d_velocity.x += cosf(this->angle + pi32/2.0f) * left;    
    player_d_velocity.y += sinf(this->angle + pi32/2.0f) * left;
    player_d_velocity = normalize(player_d_velocity);
    
    player_d_velocity *= player_speed * input->dt_per_frame;
    this->velocity = lerp(this->velocity, player_d_velocity, lerp_constant);
    
    if (input->mouse.down && this->weapon_cd_counter == 0.0f)
    {
	this->weapon_cd_counter = this->weapon_cd;

	this->has_fired = true;
    }
    else
    {
	this->has_fired = false;
    }
    
    real32 player_delta_angle = -input->mouse.dx / 500.0f * pi32/3.0f * mouse_sensitivity;
    this->angle += player_delta_angle;
    recanonicalize_angle(&this->angle);
}

internal void
simulate_world(Game_State *game_state, Game_Input *input)
{
    real32 dt = input->dt_per_frame;
    Player *player = &game_state->player;

    player->input_process(input);

    if (player->weapon_cd_counter != 0)
    {
	player->weapon_cd_counter -= (dt < player->weapon_cd_counter? dt: player->weapon_cd_counter);
    }
    player->position += player->velocity;

    for (int32 i = 0; i < game_state->entity_list.count; ++i)
    {
	Entity *entity = &game_state->entity_list.content[i];

	switch (entity->type)
	{
	    case guard:
	    case ss:
	    {
		if (entity->hp)
		{
		    entity->angle += pi32 * dt;
		    recanonicalize_angle(&entity->angle);
		}
		else
		{
		    entity->death_timer += input->dt_per_frame;
		}
	    } break;
	}
    }

    if (player->has_fired) 
    {
	game_state->need_to_play_pistol_sound = true;
	
	if (game_state->currently_aimed_entity != 0 && game_state->currently_aimed_entity->hp != 0)
	{
	    --game_state->currently_aimed_entity->hp;
	}
    }
}
