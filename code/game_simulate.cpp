#include "game_simulate.h"

internal void
weapon_start_reload(Weapon *weapon)
{
    if (weapon->bank_ammo > 0)
    {
        weapon->is_reloading = true;
        weapon->reload_time = weapon->max_reload_time;
    }
}

internal void
weapon_end_reload(Weapon *weapon)
{
    weapon->is_reloading = false;

    int32 ammo_to_refill = weapon->cache_max_ammo - weapon->cache_ammo;
    if (ammo_to_refill > weapon->bank_ammo)
    {
        ammo_to_refill = weapon->bank_ammo;
    }
    
    weapon->bank_ammo -= ammo_to_refill;
    weapon->cache_ammo += ammo_to_refill;
    weapon->reload_time = 0.0f;
}

internal void
initialize_pistol(Weapon *weapon)
{
    weapon->animation_index = 1;
    weapon->type = pistol;
    weapon->cd = 0.3f;
    
    weapon->max_reload_time = 1.0f;
    weapon->cache_max_ammo = weapon->cache_ammo = 8;
    weapon->bank_ammo = 32;
}

internal void
initialize_rifle(Weapon *weapon)
{
    weapon->animation_index = 1;
    weapon->type = rifle;
    weapon->cd = 0.1f;
    
    weapon->max_reload_time = 1.5f;
    weapon->cache_max_ammo = weapon->cache_ammo = 30;
    weapon->bank_ammo = 90;
}

internal void
initialize_minigun(Weapon *weapon)
{
    weapon->animation_index = 1;
    weapon->type = minigun;
    weapon->cd = 0.09f;
    
    weapon->max_reload_time = 1.0f;
    weapon->cache_max_ammo = weapon->cache_ammo = 100;
    weapon->bank_ammo = 300;
}

internal void
initialize_player(Player *player)
{
    player->hp = PLAYER_MAX_HP;
    
    player->position = {3.0f, 3.0f};
    player->angle = 0.0f;
    player->collision_radius = 0.3f;
    
    initialize_rifle(&player->weapon);
}

internal void
player_input_process(Player *player, Game_Input *input)
{
    //movement
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
        
        //orientation
        {
            real32 player_delta_angle = -input->mouse.dx / 500.0f * pi32/3.0f * mouse_sensitivity; //NOTE(chen): I don't know what this crap is, fix that maybe?
            player->angle += player_delta_angle;
            recanonicalize_angle(&player->angle);
        }
    }
    
    //firing system
    {
        Weapon *weapon = &player->weapon;
        
        if (weapon->is_reloading == false && input->keyboard.R)
        {
            weapon_start_reload(weapon);
        }
        else
        {
            if (weapon->is_reloading)
            {
                weapon->reload_time = reduce(weapon->reload_time, input->dt_per_frame);
                if (weapon->reload_time <= 0.0f)
                {
                    weapon_end_reload(weapon);
                }
            }
            else
            {
                if (input->mouse.down && weapon->cd_counter == 0.0f)
                {
                    if (weapon->cache_ammo > 0)
                    {
                        weapon->cd_counter = weapon->cd;
                        player->has_fired = true;
                        weapon->cache_ammo -= 1;
                    }
                    //reload
                    else
                    {
                        weapon_start_reload(weapon);
                    }
                }
                else
                {
                    player->has_fired = false;
                }
            }
        }
    }
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
    entity->clock[entity->state] = timer;
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
    real32 *clock = entity->clock;
    real32 hurting_state_interval = 0.1f;
    real32 waiting_state_interval = 1.5f;
    real32 firing_state_interval = 0.3f;
    real32 fire_preparing_interval = 0.22f;
    real32 fov = pi32 / 3.0f;
    
    if (entity->hp > 0)
    {
        //prestate processing stage
        if (entity->is_damaged)
        {
            enter_state(entity, hurting_state, hurting_state_interval);
        }
        
        //state processing stage
        bool32 clock_ends = (clock[entity->state] == 0.0f);
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
                
                bool32 not_initialized = clock[entity->state] == 0.0f;
                
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
            clock[entity->state] += dt;
        }
        else
        {
            clock[entity->state] = reduce(clock[entity->state], dt);
        }
    }
    else
    {
        entity->state = death_state;
        clock[death_state] += dt;
    }

    entity->is_damaged = false;
}

internal bool
line_vs_circle(Line_Segment L, Circle C)
{
    v2 a_to_b = L.end - L.start;
    v2 a_to_c = C.position - L.start;

    real32 a_to_b_theta = get_angle(a_to_b);
    real32 a_to_c_theta = get_angle(a_to_c);
    real32 delta_theta = get_angle_diff(a_to_c_theta, a_to_b_theta);
    
    real32 a_to_c_proj_len = cosf(delta_theta) * len(a_to_c);
    if (a_to_c_proj_len <= len(a_to_b))
    {
        v2 a_to_c_proj = normalize(a_to_b) * a_to_c_proj_len;

        v2 closest_point = L.start + a_to_c_proj;
        if (len(C.position - closest_point) <= C.radius)
        {
            return true;
        }
    }
    return false;
}

//
//
//
        
internal void
simulate_world(Game_State *game_state, Game_Input *input)
{
    real32 dt = input->dt_per_frame;
    Player *player = &game_state->player;
    
    //update player
    {
        player_input_process(player, input);    
        if (player->weapon.cd_counter != 0)
        {
            player->weapon.cd_counter -= (dt < player->weapon.cd_counter? dt: player->weapon.cd_counter);
        }
        player->position += Movement_Search_Wall(&game_state->tile_map, player, player->velocity);
    }
    
    //update entities
    for (int32 i = 0; i < game_state->entity_buffer.count; ++i)
    {
        Entity *entity = &game_state->entity_buffer.e[i];
        switch (entity->type)
        {
            case guard:
            case ss:
            {
                tick_entity_by_state(entity, &game_state->tile_map, player->position, dt);
            } break;
        }
    }
    
    //check for player being hit by bullets
    for (int32 i = 0; i < game_state->entity_buffer.count; ++i)
    {
        Entity *entity = &game_state->entity_buffer.e[i];
        if (entity->state == aiming_state)
        {
            Aiming_State *aiming_state = (Aiming_State *)entity->variant_block.storage;
            if (aiming_state->just_fired)
            {
                Line_Segment bullet_line = {};
                bullet_line.start = entity->position;
                bullet_line.end = cast_ray(&game_state->tile_map, entity->position, entity->angle).hit_position;
                
                Circle player_hitbox = {};
                player_hitbox.position = player->position;
                player_hitbox.radius = player->collision_radius;

                if (line_vs_circle(bullet_line, player_hitbox))
                {
                    if (player->hp > 0)
                    {
                        player->hp -= 1;
                    }
                }
            }
        }
    }
    
    //check which entities is damaged by player
    if (player->has_fired)
    {
        Line_Segment bullet_line = {};
        bullet_line.start = player->position;
        bullet_line.end = cast_ray(&game_state->tile_map, player->position, player->angle).hit_position;
        
        for (int i = 0; i < game_state->entity_buffer.count; ++i)
        {
            Entity *entity = &game_state->entity_buffer.e[i];
            
            Circle entity_hitbox = {};
            entity_hitbox.position = entity->position;
            entity_hitbox.radius = entity->collision_radius;
            
            if (line_vs_circle(bullet_line, entity_hitbox))
            {
                entity->hp -= 1;
                entity->is_damaged = true;
            }
        }
    }
}
