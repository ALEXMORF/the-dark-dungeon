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

internal v2
movement_search(Tile_Map *tile_map, v2 position, v2 desired_velocity, real32 radius)
{
    v2 result = {};
    v2 vital_points[8] = {};

    auto generate_vital_points = [&vital_points](v2 position, real32 radius) {
	int vital_points_count = array_count(vital_points);
	
	for (int i = 0; i < vital_points_count; ++i)
	{
	    vital_points[i] = position;
	}

	//orthogonals
	vital_points[0].x -= radius;
	vital_points[1].x += radius;
	vital_points[2].y += radius;
	vital_points[3].y -= radius;

	//diaognals
	vital_points[4] += {cosf(pi32/4.0f)*radius, sinf(pi32/4.0f)*radius};
	vital_points[5] += {-cosf(pi32/4.0f)*radius, sinf(pi32/4.0f)*radius};
	vital_points[6] += {-cosf(pi32/4.0f)*radius, -sinf(pi32/4.0f)*radius};
	vital_points[7] += {cosf(pi32/4.0f)*radius, -sinf(pi32/4.0f)*radius};
    };
    
    auto vital_points_collided = [tile_map, &vital_points]() -> bool32 {
	for (int i = 0; i < array_count(vital_points); ++i)
	{
	    if (get_tile_value(tile_map, (int32)vital_points[i].x, (int32)vital_points[i].y) != 0)
	    {
		return true;
	    }
	}
	return false;
    };
    
    //horizontal checking
    {
	v2 new_position = position;
	new_position.x += desired_velocity.x;
	generate_vital_points(new_position, radius);
	if (!vital_points_collided())
	{
	    result.x = desired_velocity.x;
	    position.x += desired_velocity.x;
	}
    }
    
    //vertical checking
    {
	v2 new_position = position;
	new_position.y += desired_velocity.y;
	generate_vital_points(new_position, radius);
	if (!vital_points_collided())
	{
	    result.y = desired_velocity.y;
	}
    }

    return result;
}

//
//
//

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

    real32 collision_radius = 0.3f;
    player->velocity = movement_search(&game_state->tile_map, player->position, player->velocity, collision_radius);
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