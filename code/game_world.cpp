#include "game_world.h"

v2
Wrapped_V2_Array::get_next()
{
    v2 result = e[current_index++];
    current_index %= length;
    return result;
}

internal void
set_room(Tile_Map *tile_map, Rect *room, Tile_Type tile_type)
{
    //NOTE(chen): since room represents empty space exclusively from walls, then expand
    //            room rect by 1

    Rect wall_rect = *room;
    wall_rect.min += {-1, -1};
    wall_rect.max += {1, 1};
    
    for (int32 x = wall_rect.min.x; x <= wall_rect.max.x; ++x)
    {
        if (get_tile_value(tile_map, x, wall_rect.min.y))
        {
            get_tile_value(tile_map, x, wall_rect.min.y) = tile_type;
        }

        if (get_tile_value(tile_map, x, wall_rect.max.y))
        {
            get_tile_value(tile_map, x, wall_rect.max.y) = tile_type;
        }
    }
    for (int32 y = wall_rect.min.y; y <= wall_rect.max.y; ++y)
    {
        if (get_tile_value(tile_map, wall_rect.min.x, y))
        {
            get_tile_value(tile_map, wall_rect.min.x, y) = tile_type;
        }

        if (get_tile_value(tile_map, wall_rect.max.x, y))
        {
            get_tile_value(tile_map, wall_rect.max.x, y) = tile_type;
        }
    }
}

inline v2
get_random_point(Rect *room)
{
    v2 result = {};
    
    real32 min_x = (real32)room->min.x + 1.5f;
    real32 max_x = (real32)room->max.x - 1.5f;
    real32 min_y = (real32)room->min.y + 1.5f;
    real32 max_y = (real32)room->max.y - 1.5f;

    result.x = real_quick_rand(min_x, max_x);
    result.y = real_quick_rand(min_y, max_y);

    return result;
}

#define Add_Static_Entity(entity_type, position) add_Entity(entity_buffer, make_static_entity(entity_type, position, game_asset))
#define Add_Dynamic_Entity(entity_type, position) add_Entity(entity_buffer, make_dynamic_entity(permanent_allocator, entity_type, position))
internal void
generate_room(Tile_Map *tile_map, Rect *room, Room_Type room_type, DBuffer(Entity) *entity_buffer,
              Linear_Allocator *permanent_allocator, Game_Asset *game_asset)
{
    v2 room_center = lerp(cast_to_v2(room->min), cast_to_v2(room->max), 0.5f);
    Add_Static_Entity(ENTITY_TYPE_LIGHT, room_center);
    
    v2 corner_array[] = {
        {room->min.x + 1.0f, room->min.y + 1.0f}, 
        {room->min.x + 1.0f, room->max.y - 1.0f},
        {room->max.x - 1.0f, room->max.y - 1.0f},
        {room->max.x - 1.0f, room->min.y + 1.0f}
    };
    
    Wrapped_V2_Array corners = {};
    {
        corners.e             = corner_array;
        corners.length        = array_count(corner_array);
        corners.current_index = ranged_rand(0, corners.length);
    }
    
    switch (room_type)
    {
        case ROOM_TYPE_BEGIN_ROOM:
        {
            Add_Static_Entity(ENTITY_TYPE_PISTOL_AMMO, corners.get_next());
            Add_Static_Entity(ENTITY_TYPE_RIFLE_AMMO, corners.get_next());
            Add_Static_Entity(ENTITY_TYPE_PISTOL_AMMO, corners.get_next());
            Add_Static_Entity(ENTITY_TYPE_HEALTHPACK, corners.get_next());

            set_room(tile_map, room, TILE_TYPE_BLUESTONE);
        } break;
        
        case ROOM_TYPE_GUARD_ROOM:
        {
            loop(3)
            {
                Add_Dynamic_Entity(ENTITY_TYPE_GUARD, corners.get_next());
            }
            Add_Static_Entity(ENTITY_TYPE_TABLE, get_random_point(room));
        } break;
        
        case ROOM_TYPE_SS_ROOM:
        {
            loop(3)
            {
                Add_Dynamic_Entity(ENTITY_TYPE_SS, corners.get_next());
            }
            Add_Static_Entity(ENTITY_TYPE_KNIGHT, get_random_point(room));
        } break;
        
        case ROOM_TYPE_SUPPLY_ROOM:
        {
            Add_Static_Entity(ENTITY_TYPE_PISTOL_AMMO, corners.get_next());
            Add_Static_Entity(ENTITY_TYPE_HEALTHPACK, corners.get_next());
            Add_Static_Entity(ENTITY_TYPE_MINIGUN_AMMO, corners.get_next());
            Add_Static_Entity(ENTITY_TYPE_RIFLE_AMMO, corners.get_next());
            Add_Static_Entity(ENTITY_TYPE_LAMP, get_random_point(room));
            
            set_room(tile_map, room, TILE_TYPE_PURPLESTONE);
        } break;
        
        case ROOM_TYPE_TORTURE_ROOM:
        {
            Add_Static_Entity(ENTITY_TYPE_CAGED_SKELETON, corners.get_next());
            Add_Static_Entity(ENTITY_TYPE_HUNG_SKELETON, corners.get_next());
            Add_Static_Entity(ENTITY_TYPE_CAGE, corners.get_next());
            Add_Static_Entity(ENTITY_TYPE_CAGED_SKELETON, corners.get_next());
            Add_Static_Entity(ENTITY_TYPE_KNIGHT, get_random_point(room));

            loop(3)
            {
                Add_Static_Entity(ENTITY_TYPE_CAGED_SKELETON, get_random_point(room));
            }
            
            Add_Dynamic_Entity(ENTITY_TYPE_SS, room_center);
            
            set_room(tile_map, room, TILE_TYPE_MOSSY);
        } break;
        
        case ROOM_TYPE_HEAVY_GUARD_ROOM:
        {
            Add_Dynamic_Entity(ENTITY_TYPE_GUARD, corners.get_next());
            Add_Dynamic_Entity(ENTITY_TYPE_SS, corners.get_next());
            Add_Dynamic_Entity(ENTITY_TYPE_GUARD, corners.get_next());
            Add_Dynamic_Entity(ENTITY_TYPE_SS, corners.get_next());
            Add_Static_Entity(ENTITY_TYPE_CAGED_SKELETON, get_random_point(room));
            
            set_room(tile_map, room, TILE_TYPE_EAGLE);
        } break;
        
        default:
        {
            assert(!"unknown room type");
        } break;
    }
}

internal void
generate_world(World *world, DBuffer(Entity) *entity_buffer,
               Game_Asset *game_asset,
               Linear_Allocator *permanent_allocator,
               Linear_Allocator *transient_allocator)
{
    Tile_Map *tile_map = &world->tile_map;
    
    if (!tile_map->tiles)
    {
        tile_map->tile_count_x = 40;
        tile_map->tile_count_y = 40;
        tile_map->exception_tile_value = 1;
        int32 tile_count = tile_map->tile_count_x * tile_map->tile_count_y;
        tile_map->tiles = Push_Array(permanent_allocator, tile_count, uint32);
    }
    
    Tile_Map_Generator generator = {};
    generator.run(tile_map, transient_allocator);
    
    //NOTE(chen): flip tile value since Tile_Map_Generator and Tile_Map has flipped representation
    //            for solid rock
    //            Tile_Map_Generator: solid wall -> 0, empty space => 1
    //            Tile_Map:           solid wall -> !0, empty space => 0
    loop_for(x, tile_map->tile_count_x)
    {
        loop_for(y, tile_map->tile_count_y)
        {
            if (get_tile_value(tile_map, x, y) == 0)
            {
                get_tile_value(tile_map, x, y) = 1;
            }
            else
            {
                get_tile_value(tile_map, x, y) = 0;
            }
        }
    }

    //clear entity buffer if not empty
    if (entity_buffer->count != 0)
    {
        entity_buffer->count = 0;
    }
    
    if (generator.room_count > 0)
    {
        v2 room_min = cast_to_v2(generator.rooms[0].min);
        v2 room_max = cast_to_v2(generator.rooms[0].max);
        v2 player_spawn_position = lerp(room_min, room_max, 0.5f);
        player_spawn_position += {0.5f, 0.5f};
        initialize_player(&world->player, player_spawn_position);
        
        for (int32 i = 0; i < generator.room_count; ++i)
        {
            Room_Type room_type;
            if (i == 0)
            {
                room_type = ROOM_TYPE_BEGIN_ROOM;
            }
            else
            {
                room_type = (Room_Type)ranged_rand(ROOM_TYPE_BEGIN_ROOM+1, ROOM_TYPE_COUNT);
            }
            
            generate_room(tile_map, &generator.rooms[i], room_type, entity_buffer,
                          permanent_allocator, game_asset);
        }
    }
    else
    {
        assert(!"no room generated, exception");
    }
}

inline bool32
Rect::collides(Rect *other_rect, int32 min_dist = 0)
{
    bool32 h_overlap = max.x >= (other_rect->min.x - min_dist) && min.x <= (other_rect->max.x + min_dist);
    bool32 v_overlap = max.y >= (other_rect->min.y - min_dist) && min.y <= (other_rect->max.y + min_dist);
    return h_overlap && v_overlap;
}

void
Tile_Map_Generator::run(Tile_Map *in_tile_map, Linear_Allocator *transient_allocator)
{
    seed_rand((uint32)time(0));

    tile_map = in_tile_map;
    region_id = TILE_VALUE_FILLER + 1;
    this->x_count = tile_map->tile_count_x;
    this->y_count = tile_map->tile_count_y;
    tiles = tile_map->tiles;
    
    room_count = 0;
    
    //zero out the entire map first
    loop_for(x, tile_map->tile_count_x)
    {
        loop_for(y, tile_map->tile_count_y)
        {
            get_tile_value(tile_map, x, y) = 0;
        }
    }
    
    //fill up with rooms
    {   
        int32 room_fill_count = MAX_ROOM;
        int32 room_max_width = 10;
        int32 room_min_width = 5;
        int32 room_max_height = 10;
        int32 room_min_height = 5;
        int32 room_min_gap = 1;
        
        loop_for(i, room_fill_count)
        {
            v2i room_size = {ranged_rand(room_min_width, room_max_width),
                             ranged_rand(room_min_height, room_max_height)};
            v2i room_lowerleft = {ranged_rand(1, x_count - room_size.x - 1),
                                  ranged_rand(1, y_count - room_size.y - 1)};
            Rect room = {room_lowerleft, room_lowerleft + room_size};
            
            bool32 room_overlaps = false;
            loop_for(room_index, room_count)
            {
                if (rooms[room_index].collides(&room, room_min_gap))
                {
                    room_overlaps = true;
                    break;
                }
            }
            
            if (!room_overlaps)
            {
                //add room
                rooms[room_count++] = room;
                for (int32 x = room.min.x; x <= room.max.x; ++x)
                {
                    for (int32 y = room.min.y; y <= room.max.y; ++y)
                    {
                        get_tile_value(tile_map, x, y) = region_id;
                    }
                }
                ++region_id;
            }
        }
    }
    
    //flood-fill corridors
    loop_for(x, x_count)
    {
        loop_for(y, y_count)
        {
            if (is_tile_walkable({x, y}))
            {
                flood_fill({x, y}, v2i_zero());
                ++region_id;
            }
        }
    }
    
    //carving ports to rooms and corridors
    {
        loop_for(room_index, room_count)
        {
            Rect *room = &rooms[room_index];
            int32 room_width = room->max.x - room->min.x;
            int32 room_height = room->max.y - room->min.y;
            int32 current_room_region_id = get_tile_value(tile_map, room->min);
                
            bool32 has_ports = true;
            while (has_ports)
            {
                v2i *possible_ports = Push_Array(transient_allocator,
                                                 (room_width-2)*2 + (room_height-2)*2, v2i);
                int32 ports_count = 0;
                
                //bottom & top row check
                for (int32 x = room->min.x + 1; x < room->max.x - 1; ++x)
                {
                    int32 bot_y = room->min.y - 1;
                    int32 top_y = room->max.y + 1;
                    v2i top_tile_position = {x, room->max.y};
                    v2i bot_tile_position = {x, room->min.y};

                    //check bot
                    if (is_tile_valid({x, bot_y-1}) && get_tile_value(tile_map, {x, bot_y-1}) != 0 &&
                        get_tile_value(tile_map, {x, bot_y-1}) != get_tile_value(tile_map, bot_tile_position))
                    {
                        possible_ports[ports_count++] = make_v2i(x, bot_y);
                    }
                    //check top
                    if (is_tile_valid({x, top_y+1}) && get_tile_value(tile_map, {x, top_y+1}) != 0 &&
                        get_tile_value(tile_map, {x, top_y+1}) != get_tile_value(tile_map, top_tile_position))
                    {
                        possible_ports[ports_count++] = make_v2i(x, top_y);
                    }
                }
                
                //left & right row check
                for (int32 y = room->min.y + 1; y < room->max.y - 1; ++y)
                {
                    int32 left_x = room->min.x - 1;
                    int32 right_x = room->max.x + 1;
                    v2i left_tile_position = {room->min.x, y};
                    v2i right_tile_position = {room->max.x, y};
                    
                    //check left
                    if (is_tile_valid({left_x-1, y}) && get_tile_value(tile_map, {left_x-1, y}) != 0 &&
                        get_tile_value(tile_map, {left_x-1, y}) != get_tile_value(tile_map, left_tile_position))
                    {
                        possible_ports[ports_count++] = make_v2i(left_x, y);
                    }
                    //check right
                    if (is_tile_valid({right_x+1, y}) && get_tile_value(tile_map, {right_x+1, y}) != 0 &&
                        get_tile_value(tile_map, {right_x+1, y}) != get_tile_value(tile_map, right_tile_position))
                    {
                        possible_ports[ports_count++] = make_v2i(right_x, y);
                    }
                }

                //pick a random port and carve it 
                if (ports_count != 0)
                {
                    int32 random_index2 = ranged_rand(0, ports_count);
                    v2i port_position = possible_ports[random_index2];
                    
                    get_tile_value(tile_map, port_position) = TILE_VALUE_FILLER;
                    unify_region_id(port_position, current_room_region_id);
                }
                else
                {
                    has_ports = false;
                }
            }
        }
    }
    
    //uncarving deadends
    loop_for(x, x_count)
    {
        loop_for(y, y_count)
        {
            v2i tile_position = {x, y};
            if (get_tile_value(tile_map, tile_position) != 0)
            {
                int32 sides_connected = 0;
                v2i side_offsets[] = {{-1, 0}, {1, 0}, {0, 1}, {0, -1}};
                for_each(i, side_offsets)
                {
                    if (is_tile_valid(tile_position + side_offsets[i]) &&
                        get_tile_value(tile_map, tile_position + side_offsets[i]) != 0)
                    {
                        ++sides_connected;
                    }
                }
                
                if (sides_connected == 0)
                {
                    get_tile_value(tile_map, tile_position) = 0;
                }
                else if (sides_connected == 1)
                {
                    uncarve(tile_position);
                }
            }
        }
    }
}

void
Tile_Map_Generator::uncarve(v2i tile_position)
{
    if (get_tile_value(tile_map, tile_position) == 0)
    {
        return;
    }
    
    int32 sides_connected = 0;
    v2i side_offsets[] = {{-1, 0}, {1, 0}, {0, 1}, {0, -1}};
    v2i sole_side_offset = {};
    for_each(i, side_offsets)
    {
        if (is_tile_valid(tile_position + side_offsets[i]) &&
            get_tile_value(tile_map, tile_position + side_offsets[i]) != 0)
        {
            ++sides_connected;
            sole_side_offset = side_offsets[i];
        }
    }
    
    if (sides_connected == 1)
    {
        get_tile_value(tile_map, tile_position) = 0;
        uncarve(tile_position + sole_side_offset);
    }
}

void
Tile_Map_Generator::unify_region_id(v2i tile_position, uint32 in_region_id)
{
    if (is_tile_valid(tile_position) && get_tile_value(tile_map, tile_position) != 0 &&
        get_tile_value(tile_map, tile_position) != in_region_id)
    {
        get_tile_value(tile_map, tile_position) = in_region_id;
        
        unify_region_id(tile_position + make_v2i(1, 0), in_region_id);
        unify_region_id(tile_position + make_v2i(-1, 0), in_region_id);
        unify_region_id(tile_position + make_v2i(0, 1), in_region_id);
        unify_region_id(tile_position + make_v2i(0, -1), in_region_id);
    }
}

bool32
Tile_Map_Generator::is_tile_valid(v2i tile_position)
{
    bool32 x_is_valid = tile_position.x >= 0 && tile_position.x < x_count;
    bool32 y_is_valid = tile_position.y >= 0 && tile_position.y < y_count;
    return x_is_valid && y_is_valid;
}

bool32
Tile_Map_Generator::is_tile_walkable(v2i tile_position)
{
    bool32 is_walkable = true;
    for (int32 x = tile_position.x - 1; x <= tile_position.x + 1; ++x)
    {
        if (!is_walkable)
        {
            break;
        }
        for (int32 y = tile_position.y - 1; y <= tile_position.y + 1; ++y)
        {
            if (!is_tile_valid({x, y}) || get_tile_value(tile_map, {x, y}) != 0)
            {
                is_walkable = false;
                break;
            }
        }
    }
    
    return is_walkable;
}

void
Tile_Map_Generator::flood_fill(v2i tile_position, v2i flood_direction)
{
    if (!is_tile_valid(tile_position))
    {
        return;
    }
    
    //randomly shuffle next directions
    v2i directions[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int32 i = array_count(directions)-1; i >= 0; --i)
    {
        int32 random_index2 = ranged_rand(0, array_count(directions));
        
        //swap
        if (random_index2 != i)
        {
            v2i temp = directions[i];
            directions[i] = directions[random_index2];
            directions[random_index2] = temp;
        }
    }

    //check if current tile position is fillable
    bool32 tile_position_is_walkable = true;
    if (flood_direction == v2i_zero())
    {
        tile_position_is_walkable = is_tile_walkable(tile_position);
    }
    else
    {
        v2i check_positions[6];
        if (flood_direction.x)
        {
            check_positions[0] = tile_position;
            check_positions[1] = {tile_position.x, tile_position.y+1};
            check_positions[2] = {tile_position.x, tile_position.y-1};
            check_positions[3] = tile_position + flood_direction;
            check_positions[4] = check_positions[3] + make_v2i(0, -1);
            check_positions[5] = check_positions[3] + make_v2i(0, 1);
        }
        else if (flood_direction.y)
        {
            check_positions[0] = tile_position;
            check_positions[1] = {tile_position.x+1, tile_position.y};
            check_positions[2] = {tile_position.x-1, tile_position.y};
            check_positions[3] = tile_position + flood_direction;
            check_positions[4] = check_positions[3] + make_v2i(1, 0);
            check_positions[5] = check_positions[3] + make_v2i(-1, 0);
        }
        else
        {
            assert(!"what the fuck floor_direction is nulled???");
        }
        for_each(i, check_positions)
        {
            if (!is_tile_valid(check_positions[i]) ||
                get_tile_value(tile_map, check_positions[i]) != 0)
            {
                tile_position_is_walkable = false;
                break;
            }
        }
    }
    
    //fill tile and flood surrounding tiles
    if (tile_position_is_walkable)
    {
        get_tile_value(tile_map, tile_position) = region_id;
        for_each(i, directions)
        {
            flood_fill(tile_position + directions[i], directions[i]);
        }
    }
}
