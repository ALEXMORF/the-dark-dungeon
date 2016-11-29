#pragma once

enum Weapon_Type
{
    knife, 
    pistol,
    rifle,
    minigun,
    weapon_type_count
};

#define PLAYER_MAX_HP 10

struct Weapon
{
    Weapon_Type type;

    real32 force;
    
    real32 cd;
    real32 cd_counter;
    
    bool32 is_reloading;
    real32 max_reload_time;
    real32 reload_time;
    int32 cache_max_ammo;
    int32 cache_ammo;
    int32 bank_ammo;
    
    int32 animation_index;
};

struct Player
{
    //coordniate
    Rigid_Body body;
    real32 angle;
    
    real32 pace;
    
    //NOTE(chen): for weapon animation
    real32 weapon_reload_offset;
    
    //record
    bool32 has_fired;
    int32 hp;
    
    //weapon system
    int32 weapon_index;
    Weapon weapons[weapon_type_count];
    
    Weapon *get_weapon()
    {
        return &weapons[weapon_index];
    }
};
