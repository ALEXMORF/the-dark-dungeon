#pragma once

enum Entity_Type
{
    barrel,
    pillar,
    guard,
    ss, 
    entity_type_count
};

enum Entity_State
{
    waiting_state,    
    walking_state,
    hurting_state,
    aiming_state,
    shooting_state,
    death_state,
    entity_state_count
};

struct Entity
{
    Entity_Type type;
    Entity_State state;
    real32 clock[entity_state_count];
    
    v2 position;
    real32 angle;
    real32 collision_radius;
    
    real32 speed;
    v2 destination;
    
    int32 hp;
    bool32 is_damaged;
    
    bool variant_block_is_initialized;
    Memory variant_block;
};

