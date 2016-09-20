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
    aiming_state
};

struct Entity_Clock
{
    real32 death_timer;
    real32 walk_timer;
    real32 wait_timer;
    real32 hurt_timer;
};

struct Entity
{
    Entity_Type type;
    Entity_State state;
    Entity_Clock clock;
    
    //coordinate
    v2 position;
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

