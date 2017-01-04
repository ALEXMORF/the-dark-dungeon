#pragma once

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

