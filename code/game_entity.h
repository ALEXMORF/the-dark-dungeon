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
    walking_state,
    waiting_state,
    hurting_state,
    aiming_state,
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
    v2 velocity;
    v2 destination;
    real32 angle;
    bool32 walking;
    
    //stats
    int32 hp;
    bool32 just_shot;
};

struct Entity_List
{
    Entity *content;
    int32 count;
    int32 capacity;
};

