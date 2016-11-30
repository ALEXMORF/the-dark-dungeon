#pragma once

enum Entity_Type
{
    //dynamic entity types
    guard,
    ss,

    //static entity types
    light,
    lamp,
    
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

    Rigid_Body body;
    real32 angle;
    
    real32 speed;
    v2 destination;
    
    int32 hp;
    real32 weapon_force;
    bool32 is_damaged;
    
    bool variant_block_is_initialized;
    Memory variant_block;

    Loaded_Image sprite;
};

