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
