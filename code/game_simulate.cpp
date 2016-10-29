#include "game_simulate.h"

internal void
player_input_process(Player *player, Game_Input *input)
{
    real32 player_speed = 2.5f;
    real32 lerp_constant = 0.15f;
    real32 mouse_sensitivity = 0.7f;

    real32 forward = 0.0f;
    real32 left = 0.0f;
    if_do(input->keyboard.left, left = 1.0f);
    if_do(input->keyboard.right, left = -1.0f);
    if_do(input->keyboard.up, forward = 1.0f);
    if_do(input->keyboard.down, forward = -1.0f);
    
    v2 player_d_velocity = {};
    player_d_velocity += {cosf(player->angle) *forward, sinf(player->angle) * forward};    
    player_d_velocity += {cosf(player->angle + pi32/2.0f) * left, sinf(player->angle + pi32/2.0f) * left};    
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
                       
#define Movement_Search_Wall(tile_map, entity, velocity) movement_search_wall(tile_map, entity->position, velocity, entity->collision_radius)
internal v2
movement_search_wall(Tile_Map *tile_map, v2 position, v2 desired_velocity, real32 radius)
{
    v2 result = {};
    v2 vital_points[8] = {};
    
    auto generate_vital_points = [&vital_points](v2 position, real32 radius) {
        int vital_points_count = array_count(vital_points);
        for (int i = 0; i < vital_points_count; ++i)
        {
            vital_points[i] = position;
        }
        vital_points[0].x -= radius;
        vital_points[1].x += radius;
        vital_points[2].y += radius;
        vital_points[3].y -= radius;
        vital_points[4] += {cosf(pi32/4.0f)*radius, sinf(pi32/4.0f)*radius};
        vital_points[5] += {-cosf(pi32/4.0f)*radius, sinf(pi32/4.0f)*radius};
        vital_points[6] += {-cosf(pi32/4.0f)*radius, -sinf(pi32/4.0f)*radius};
        vital_points[7] += {cosf(pi32/4.0f)*radius, -sinf(pi32/4.0f)*radius};
    };
    auto vital_points_collided = [tile_map, &vital_points]() -> bool32 {
        for (int i = 0; i < array_count(vital_points); ++i)
        {
            if (get_tile_value(tile_map, (int32)vital_points[i].x, (int32)vital_points[i].y) != 0) {
                return true;
            }
        }
        return false;
    };
    
    v2 new_position = position;
    new_position.x += desired_velocity.x;
    generate_vital_points(new_position, radius);
    if (!vital_points_collided())
    {
        result.x = desired_velocity.x;
        position.x += desired_velocity.x;
    }
    new_position = position;
    new_position.y += desired_velocity.y;
    generate_vital_points(new_position, radius);
    if (!vital_points_collided())
    {
        result.y = desired_velocity.y;
    }
    
    return result;
}

inline void
enter_state(Entity *entity, Entity_State next_state, real32 timer)
{
    entity->state = next_state;
    entity->clock.timer[entity->state] = timer;
    entity->variant_block_is_initialized = false;
}

inline bool32
search_player(Tile_Map *tile_map, Entity *entity, v2 player_position, real32 fov)
{
    real32 angle_player = get_angle(player_position - entity->position);
    real32 angle_diff = get_angle_diff(angle_player, entity->angle);
    real32 player_to_entity_dist = len(player_position - entity->position);
    recanonicalize_angle(&angle_player);
    real32 ray_length = cast_ray(tile_map, entity->position, angle_player).ray_length;
                    
    bool32 within_fov = (abs(angle_diff) <= fov/2.0f);
    bool32 visible = ray_length > player_to_entity_dist;

    return (within_fov && visible);
}
//
//
//

internal void
tick_entity_by_state(Entity *entity, Tile_Map *tile_map, v2 player_position, real32 dt)
{
    Entity_Clock *clock = &entity->clock;
    real32 hurting_state_interval = 0.1f;
    real32 waiting_state_interval = 1.5f;
    real32 firing_state_interval = 0.3f;
    real32 fire_preparing_interval = 0.22f;
    real32 fov = pi32 / 3.0f;
    
    if (entity->hp > 0)
    {
        //prestate processing stage
        if (entity->just_got_shot)
        {
            enter_state(entity, hurting_state, hurting_state_interval);
        }

        //state processing stage
        bool32 clock_ends = (clock->timer[entity->state] == 0.0f);
        bool32 clock_ticks_forward = false;
        switch (entity->state)
        {
            case hurting_state:
            {
                if (clock_ends)
                {
                    entity->angle = get_angle(player_position - entity->position);
                    enter_state(entity, waiting_state, waiting_state_interval);
                }
            } break;
            
            case walking_state:
            {
                clock_ticks_forward = true;
                
                bool32 not_initialized = clock->timer[entity->state] == 0.0f;
                
                if (search_player(tile_map, entity, player_position, fov))
                {
                    enter_state(entity, aiming_state, 0.0f);
                }
                if (not_initialized)
                {
                    entity->angle += pi32 / 3.0f;
                    recanonicalize_angle(&entity->angle);
                    entity->destination = cast_ray(tile_map, entity->position, entity->angle).hit_position;
                }

                real32 distance_left = len(entity->destination - entity->position);
                real32 speed = clamp(entity->speed, 0.0f, distance_left);
                v2 displacement = normalize(entity->destination - entity->position) * speed * dt;
                displacement = Movement_Search_Wall(tile_map, entity, displacement);
                entity->position += displacement;
                if (speed < entity->speed || len(displacement)/dt < speed)
                {
                    enter_state(entity, waiting_state, waiting_state_interval);
                }
            } break;
            
            case waiting_state:
            {
                if (search_player(tile_map, entity, player_position, fov))
                {
                    enter_state(entity, aiming_state, 0.0f);
                }
                if (clock_ends)
                {
                    enter_state(entity, walking_state, 0.0f);
                }
            } break;

            case aiming_state:
            {
                clock_ticks_forward = true;
                
                assert(sizeof(Aiming_State) <= entity->variant_block.size);
                Aiming_State *aiming_state = (Aiming_State *)entity->variant_block.storage;
                if (!entity->variant_block_is_initialized)
                {
                    //startup code
                    switch (entity->type)
                    {
                        case guard:
                        {
                            aiming_state->allowed_firing_interval = 1.5f;
                        } break;

                        case ss:
                        {
                            aiming_state->allowed_firing_interval = 1.0f;
                        } break;
                    }
                    aiming_state->allowed_firing_animation_cd = 0.1f;
                    aiming_state->firing_animation_cd = 0.0f;
                    aiming_state->firing_timer = 0.0f;

                    entity->variant_block_is_initialized = true;
                }

                //scan for player
                entity->angle = get_angle(player_position - entity->position);
                recanonicalize_angle(&entity->angle);
                if (!search_player(tile_map, entity, player_position, fov))
                {
                    enter_state(entity, walking_state, 0.0f);
                }
                
                //firing system
                aiming_state->firing_timer += dt;
                if (aiming_state->firing_timer > aiming_state->allowed_firing_interval)
                {
                    aiming_state->firing_timer -= aiming_state->allowed_firing_interval;
                    aiming_state->just_fired = true;
                    aiming_state->firing_animation_cd = aiming_state->allowed_firing_animation_cd;
                }
                else
                {
                    aiming_state->just_fired = false;
                    if (aiming_state->firing_animation_cd > 0)
                    {
                        aiming_state->firing_animation_cd = reduce(aiming_state->firing_animation_cd, dt);
                    }
                }
            } break;
        }

        if (clock_ticks_forward)
        {
            clock->timer[entity->state] += dt;
        }
        else
        {
            clock->timer[entity->state] = reduce(clock->timer[entity->state], dt);
        }
    }
    else
    {
        entity->state = death_state;
        clock->timer[death_state] += dt;
    }

    entity->just_got_shot = false;
}

//
//
//

internal void
simulate_world(Game_State *game_state, Game_Input *input)
{
    real32 dt = input->dt_per_frame;
    Player *player = &game_state->player;

    player_input_process(player, input);    
    if (player->weapon_cd_counter != 0)
    {
        player->weapon_cd_counter -= (dt < player->weapon_cd_counter? dt: player->weapon_cd_counter);
    }
    player->position += Movement_Search_Wall(&game_state->tile_map, player, player->velocity);
    
    for (int32 i = 0; i < game_state->entity_list.count; ++i)
    {
        Entity *entity = &game_state->entity_list.content[i];
        switch (entity->type)
        {
            case guard:
            case ss:
            {
                tick_entity_by_state(entity, &game_state->tile_map, player->position, dt);
            } break;
        }
    }

    if (player->has_fired)
    {
        if (game_state->currently_aimed_entity != 0 && game_state->currently_aimed_entity->hp != 0)
        {
            Entity *entity_shot = game_state->currently_aimed_entity;
            --entity_shot->hp;
            entity_shot->just_got_shot = true;
        }
    }
}
