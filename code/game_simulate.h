#pragma once

//states
struct Aiming_State
{
    real32 allowed_firing_interval;
    real32 firing_timer;
    real32 allowed_firing_animation_cd;
    real32 firing_animation_cd;
    bool just_fired;
};

struct Line_Segment
{
    v2 start;
    v2 end;
};

struct Circle
{
    v2 position;
    real32 radius;
};

struct Rigid_Body
{
    v2 position;
    v2 velocity;
    real32 collision_radius;
    real32 mass;
    
    v2 velocity_to_apply;
    v2 force_to_apply;
};

