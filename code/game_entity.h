#pragma once

enum Entity_Type
{
    barrel,
    pillar,
    guard,
    ss, 
    entity_type_count
};

enum Weapon
{
    knife,
    pistol,
    rifle,
    minigun,
    weapon_count
};

struct Entity
{
    Entity_Type type;

    v2 position;
    real32 angle;
    
    int32 hp;
    real32 death_timer;
};

struct Entity_List
{
    Entity *content;
    int32 count;
    int32 capacity;
};

