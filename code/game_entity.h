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

struct Entity_Clock
{
    real32 timer[entity_state_count];
};

struct Entity
{
    Entity_Type type;
    Entity_State state;
    Entity_Clock clock;
    
    v2 position;
    real32 speed;
    v2 destination;
    real32 angle;
    real32 collision_radius;
    
    int32 hp;
    bool32 just_got_shot;

    bool variant_block_is_initialized;
    Memory variant_block;
};

struct Entity_List
{
    Entity *content;
    int32 count;
    int32 capacity;
};

