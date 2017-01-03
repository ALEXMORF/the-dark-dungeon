#pragma once

enum Entity_Type
{
    //dynamic entity types
    ENTITY_TYPE_GUARD,
    ENTITY_TYPE_SS,
    
    //static entity types
    ENTITY_TYPE_HEALTHPACK,
    ENTITY_TYPE_PISTOL_AMMO,
    ENTITY_TYPE_RIFLE_AMMO,
    ENTITY_TYPE_MINIGUN_AMMO,
    
    //decorative types
    ENTITY_TYPE_BARREL,
    ENTITY_TYPE_TABLE,
    ENTITY_TYPE_LAMP,
    ENTITY_TYPE_LIGHT,
    ENTITY_TYPE_HUNG_SKELETON,
    ENTITY_TYPE_PLANT,
    ENTITY_TYPE_FALLEN_SKELETON,
    ENTITY_TYPE_LAMP2,
    ENTITY_TYPE_KNIGHT,
    ENTITY_TYPE_CAGE,
    ENTITY_TYPE_CAGED_SKELETON,
    ENTITY_TYPE_SKELETON_DUST,
    ENTITY_TYPE_SKELETON_BLOOD,
    
    ENTITY_TYPE_COUNT,
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
    bool32 is_static;
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

