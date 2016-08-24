#pragma once

struct Projection_Spec
{
    real32 dim;
    real32 fov;
    real32 view_distance;
};

struct Reflection_Sample
{
    bool32 x_side_faced;
    
    v2 hit_position;
    real32 ray_length;
    
    bool32 is_valid;
};
