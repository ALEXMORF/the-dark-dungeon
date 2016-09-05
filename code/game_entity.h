#pragma once

enum Entity_Type
{
    barrel,
    pillar,
    guard,
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

#define ENTITY_COUNT_MAX 50
struct Entity_List
{
    Entity content[ENTITY_COUNT_MAX];
    int32 count;
};

struct Player
{
    v2 position;
    v2 velocity;
    real32 angle;
    
    Weapon weapon;
    int32 weapon_animation_index;
    real32 weapon_cd;
    real32 weapon_cd_counter;
};

