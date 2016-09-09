#include "game_entity.h"

internal void
add_entity(Entity_List *list, Entity entity)
{
    assert(list->count+1 < list->capacity);
    list->content[list->count++] = entity;
}

inline Entity
make_static_entity(Entity_Type type, v2 position)
{
    Entity result = {};
    result.type = type;
    result.position = position;

    return result;
}

inline Entity
make_dynamic_entity(Entity_Type type, v2 position, int32 hp = 1)
{
    Entity result = {};
    result.type = type;
    result.position = position;
    result.hp = hp;
    
    return result;
}

