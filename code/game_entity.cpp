#include "game_entity.h"

inline Entity
make_static_entity(Entity_Type type, v2 position)
{
    Entity result = {};
    result.type = type;
    result.position = position;

    return result;
}

inline Entity
make_dynamic_entity(Linear_Allocator *allocator, Entity_Type type, v2 position, real32 angle = 0.0f)
{
    Entity result = {};
    result.type = type;
    result.position = position;
    result.angle = angle;
    result.collision_radius = 0.3f;
    result.speed = 1.5f;

    switch (result.type)
    {
        case guard:
        {
            result.hp = 2;
        } break;

        case ss:
        {
            result.hp = 3;
        } break;

        default:
        {
            assert(!"unrecognized dynamic entity type");
        } break;
    }

    result.variant_block.size = 50;
    result.variant_block.storage = linear_allocate(allocator, result.variant_block.size);

    return result;
}


