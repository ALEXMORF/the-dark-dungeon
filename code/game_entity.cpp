#include "game_entity.h"

inline Entity
make_static_entity(Entity_Type type, v2 position, Game_Asset *asset)
{
    Entity result = {};
    result.type = type;
    result.body = default_rigid_body(position, 0.3f);

    //TODO(chen): expand entity types
    Loaded_Image_Sheet *sprite_sheet = &asset->entity_sheet;
    switch (type)
    {
        default:
        {
            assert(!"unknown static entity type");
        } break;
    }
    return result;
}

inline Entity
make_dynamic_entity(Linear_Allocator *allocator, Entity_Type type, v2 position, real32 angle = 0.0f)
{
    Entity result = {};
    result.type = type;
    result.body = default_rigid_body(position, 0.3f);
    result.angle = angle;
    result.speed = 1.5f;

    switch (result.type)
    {
        case guard:
        {
            result.hp = 3;
            result.weapon_force = 50.0f;
        } break;

        case ss:
        {
            result.hp = 5;
            result.weapon_force = 30.0f;
        } break;

        default:
        {
            assert(!"unrecognized dynamic entity type");
        } break;
    }

    result.variant_block.size = 50;
    result.variant_block.storage = linear_allocate(allocator, result.variant_block.size);

    return result;
}

inline bool32
search_player(Tile_Map *tile_map, Entity *entity, v2 player_position, real32 fov)
{
    real32 angle_player = get_angle(player_position - entity->body.position);
    real32 angle_diff = get_angle_diff(angle_player, entity->angle);
    real32 player_to_entity_dist = len(player_position - entity->body.position);
    recanonicalize_angle(&angle_player);
    real32 ray_length = cast_ray(tile_map, entity->body.position, angle_player).ray_length;
                    
    bool32 within_fov = (abs(angle_diff) <= fov/2.0f);
    bool32 visible = ray_length > player_to_entity_dist;

    return (within_fov && visible);
}

inline void
enter_state(Entity *entity, Entity_State next_state, real32 timer)
{
    entity->state = next_state;
    entity->clock[entity->state] = timer;
    entity->variant_block_is_initialized = false;
}

internal void
update_basic_entity(Entity *entity, Tile_Map *tile_map, v2 player_position, real32 dt)
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
                    entity->angle = get_angle(player_position - entity->body.position);
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
                    entity->destination = cast_ray(tile_map, entity->body.position, entity->angle).hit_position;
                }
                
                real32 distance_left = len(entity->destination - entity->body.position);
                real32 speed = clamp(entity->speed, 0.0f, distance_left);
                v2 displacement = normalize(entity->destination - entity->body.position) * speed * dt;
                entity->body.velocity_to_apply = displacement;
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
                            aiming_state->allowed_firing_interval = 1.0f;
                        } break;
                        
                        case ss:
                        {
                            aiming_state->allowed_firing_interval = 0.5f;
                        } break;
                    }
                    aiming_state->allowed_firing_animation_cd = 0.1f;
                    aiming_state->firing_animation_cd = 0.0f;
                    aiming_state->firing_timer = 0.0f;

                    entity->variant_block_is_initialized = true;
                }

                //scan for player
                entity->angle = get_angle(player_position - entity->body.position);
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
