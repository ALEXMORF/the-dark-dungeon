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

