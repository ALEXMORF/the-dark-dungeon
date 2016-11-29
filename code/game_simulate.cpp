#include "game_simulate.h"

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

inline Rigid_Body
default_rigid_body(v2 position, real32 collision_radius, real32 mass = 1.0f)
{
    Rigid_Body result = {};
    result.position = position;
    result.collision_radius = collision_radius;
    result.mass = mass;
    return result;
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

internal void
reset_body(Rigid_Body *body)
{
    body->velocity_to_apply = {0.0f, 0.0f};
    body->force_to_apply = {0.0f};
}

//NOTE(chen): standard for force: each 10000N moves a body of mass 1KG back 1.0 world unit
internal void
simulate_body(Rigid_Body *body, Tile_Map *tile_map)
{
    real32 velocity_lerp = 0.2f;
    
    //TODO(chen): simulate entity vs entity collision

    //apply velocity
    v2 desired_velocity = lerp(body->velocity, body->velocity_to_apply, velocity_lerp);
    
    //apply force
    real32 force_constant = 10000.0f;
    desired_velocity += (body->force_to_apply / force_constant) / body->mass;
    
    //update physical representation of body
    body->velocity = Movement_Search_Wall(tile_map, body, desired_velocity);
    body->position += body->velocity;
}
