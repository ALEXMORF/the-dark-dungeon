#include "game.h"

#include "game_math.cpp"
#include "game_tiles.cpp"

#include "game_asset.cpp"
#include "game_render.cpp"
#include "game_raycaster.cpp"

#define Copy_Array(source, dest, count, type) copy_memory(source, dest, count*sizeof(type))
inline void
copy_memory(void *source_in, void *dest_in, uint32 size)
{
    uint8 *source = (uint8 *)source_in;
    uint8 *dest = (uint8 *)dest_in;
    for (uint32 i = 0; i < size; ++i)
    {
	dest[i] = source[i];
    }
}

inline void
initialize_linear_allocator(Linear_Allocator *allocator, void *base_ptr, uint32 size)
{
    allocator->base_ptr = (uint8 *)base_ptr;
    allocator->size = size;
    allocator->used = 0;
}

#define Push_Array(allocator, wanted_size, type) (type *)linear_allocate(allocator, wanted_size*sizeof(type))
inline void *
linear_allocate(Linear_Allocator *allocator, uint32 wanted_size)
{
    assert(allocator->used + wanted_size <= allocator->size);
    void *result = (uint8 *)allocator->base_ptr + allocator->used;
    allocator->used += wanted_size;
    return result;
}

internal void
load_assets(Game_State *game_state, Platform_Load_Image *platform_load_image)
{
    #define Load_Wall_Tex(index, filename) game_state->wall_textures.E[index] = load_image(platform_load_image, filename);
    game_state->wall_textures.count = 6;
    game_state->wall_textures.E = Push_Array(&game_state->permanent_allocator, game_state->wall_textures.count, Loaded_Image);
    Load_Wall_Tex(0, "../data/redbrick.png");
    Load_Wall_Tex(1, "../data/bluestone.png");
    Load_Wall_Tex(2, "../data/colorstone.png");
    Load_Wall_Tex(3, "../data/eagle.png");
    Load_Wall_Tex(4, "../data/purplestone.png");
    Load_Wall_Tex(5, "../data/wood.png");
	
    game_state->floor_texture = load_image(platform_load_image, "../data/greystone.png");
    game_state->ceiling_texture = load_image(platform_load_image, "../data/greystone.png");
    game_state->barrel_texture = load_image(platform_load_image, "../data/barrel.png");
    game_state->pillar_texture = load_image(platform_load_image, "../data/pillar.png");
    game_state->light_texture = load_image(platform_load_image, "../data/greenlight.png");

    //load and configure 1st person weapon sprite sheet
    game_state->weapon_texture_sheet = load_image_sheet(platform_load_image, "../data/weapons.png");
    game_state->weapon_texture_sheet.image_count_x = 5;
    game_state->weapon_texture_sheet.image_count_y = 5;
    game_state->weapon_texture_sheet.stride_offset = 1;
    game_state->weapon_texture_sheet.image_width = 64;
    game_state->weapon_texture_sheet.image_height = 64;
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    assert(sizeof(Game_State) <= memory->permanent_storage_size);
    Game_State *game_state = (Game_State *)memory->permanent_storage;
    
    if (!memory->is_initialized)
    {
	//prepare memory allocators
	initialize_linear_allocator(&game_state->permanent_allocator,
				    (uint8 *)memory->permanent_storage + sizeof(Game_State),
				    memory->permanent_storage_size - sizeof(Game_State));
	initialize_linear_allocator(&game_state->transient_allocator,
				    (uint8 *)memory->transient_storage,
				    memory->transient_storage_size);
	
	//game
//TODO(chen): this is some improv tile-map init code, replace this with procedural generation later
#define map_width 20
#define map_height 20
	uint32 tiles[map_width*map_height] =
	    {
		3, 3, 3, 3, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1,
		3, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		3, 0, 0, 0, 4, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		3, 0, 0, 0, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
		3, 1, 4, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 0, 0, 2, 1, 1, 1,
		1, 1, 1, 2, 0, 0, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
		4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1
	    };
	Tile_Map *tile_map = &game_state->tile_map;
	tile_map->tile_count_x = map_width;
	tile_map->tile_count_y = map_height;
	tile_map->exception_tile_value = 1;
	int32 tile_count = tile_map->tile_count_x * tile_map->tile_count_y;
	tile_map->tiles = Push_Array(&game_state->permanent_allocator, tile_count, uint32);
	Copy_Array(tiles, tile_map->tiles, tile_count, uint32);

	game_state->player.position = {3.0f, 3.0f};
	game_state->player.angle = 0.0f;
	game_state->player.weapon_animation_index = 1;
	game_state->player.weapon = pistol;
	game_state->player.weapon_cd = 0.7f;
	
	game_state->barrel_position = {5.0f, 5.0f};
	
	load_assets(game_state, memory->platform_load_image);
	
	//prepare renderer context
	Render_Context *render_context = &game_state->render_context;
	
	render_context->z_buffer = Push_Array(&game_state->permanent_allocator,
					      buffer->width, real32);
	render_context->floorcast_table_count = buffer->height/2;
	render_context->floorcast_table = Push_Array(&game_state->permanent_allocator,
						     render_context->floorcast_table_count, real32);
	for (int32 i = 0; i < render_context->floorcast_table_count; ++i)
	{
	    real32 inverse_aspect_ratio = (real32)buffer->width / buffer->height;
	    real32 real_scan_y = ((real32)buffer->height/2 + (real32)i / inverse_aspect_ratio);
	    render_context->floorcast_table[i] = ((real32)buffer->height /
						  (2*real_scan_y - buffer->height));
	}

	memory->is_initialized = true;
    }

    //NOTE(chen): transient memory gets dumped every frame
    game_state->transient_allocator.used = 0;

    Player *player = &game_state->player;
    //
    //
    //input

    real32 forward = 0.0f;
    real32 left = 0.0f;
    if (input->keyboard.left)
    {
	left = 1.0f;
    }
    if (input->keyboard.right)
    {
	left = -1.0f;
    }
    if (input->keyboard.up)
    {
	forward = 1.0f;
    }
    if (input->keyboard.down)
    {
	forward = -1.0f;
    }
    
    v2 player_delta_direction = {};
    player_delta_direction.y += sinf(game_state->player.angle) * forward;
    player_delta_direction.x += cosf(game_state->player.angle) *forward;
    player_delta_direction.y += sinf(game_state->player.angle + pi32/2.0f) * left;
    player_delta_direction.x += cosf(game_state->player.angle + pi32/2.0f) * left;
    player_delta_direction = normalize(player_delta_direction);
    
    real32 player_speed = 1.5f;
    player_delta_direction *= player_speed;
    game_state->player.position += player_delta_direction * input->dt_per_frame;

    //firing system
    if (input->mouse.down)
    {
	if (player->weapon_cd_counter == 0.0f)
	{
	    memory->platform_play_sound("../data/pistol.wav");	    
	    player->weapon_cd_counter = player->weapon_cd;
	}
    }
    
    //
    //
    //update
    
    real32 mouse_sensitivity = 0.7f;
    real32 player_delta_angle = -input->mouse.dx / 500.0f * pi32/3.0f * mouse_sensitivity;
    player->angle += player_delta_angle;
    recanonicalize_angle(&player->angle);

    //reduice cd & animate weapon
    if (player->weapon_cd_counter)
    {
	real32 time_passed = player->weapon_cd - player->weapon_cd_counter;
	real32 animation_cycle = 0.6f;
	if (time_passed < animation_cycle)
	{
	    real32 animation_index_interval = animation_cycle / 4.0f;
	    player->weapon_animation_index = (int32)(time_passed / animation_index_interval + 1) % 5;
	}
	else
	{
	    player->weapon_animation_index = 1;
	}
	
	//cool down 
	player->weapon_cd_counter -= (input->dt_per_frame < player->weapon_cd_counter ?
				      input->dt_per_frame :
				      player->weapon_cd_counter);
    }
    //
    //
    //render

    fill_buffer(buffer, 0);
    
    Sprite sprite_list[] =
    {
	{{1.0f, 1.0f}, {5.0f, 5.0f},   &game_state->light_texture},
	{{1.0f, 1.0f}, {15.0f, 5.0f},  &game_state->light_texture},
	{{1.0f, 1.0f}, {15.0f, 15.0f}, &game_state->light_texture},
	{{1.0f, 1.0f}, {7.0f, 15.0f},  &game_state->light_texture},
	    
	{{1.0f, 1.0f}, {5.5f, 4.5f},   &game_state->barrel_texture},
	{{1.0f, 1.0f}, {5.5f, 5.5f},   &game_state->barrel_texture},
	{{1.0f, 1.0f}, {5.5f, 6.5f},   &game_state->barrel_texture},

	{{1.0f, 1.0f}, {4.5f, 15.5f},  &game_state->pillar_texture},
    };
    
    sort_sprites(sprite_list, array_count(sprite_list), game_state->player.position);
    render_3d_scene(buffer, &game_state->render_context, &game_state->tile_map,
		    game_state->player.position, game_state->player.angle, 
		    &game_state->floor_texture, &game_state->ceiling_texture,
		    &game_state->wall_textures,
		    sprite_list, array_count(sprite_list));

    //draw first-person weapon
    {
	v2 weapon_sprite_size = {(real32)buffer->height, (real32)buffer->height};
	int32 weapon_upper_left = (buffer->width - (int32)weapon_sprite_size.x) / 2;
	int32 weapon_upper_top = 0;
	int32 weapon_lower_right = weapon_upper_left + (int32)weapon_sprite_size.x;
	
	Loaded_Image weapon_image = extract_image_from_sheet(&game_state->weapon_texture_sheet,
							     game_state->player.weapon_animation_index,
							     game_state->player.weapon);
	real32 texture_x = 0.0f;
	real32 texture_mapper = (real32)weapon_image.width / weapon_sprite_size.x;
	for (int32 dest_x = weapon_upper_left; dest_x < weapon_lower_right; ++dest_x)
	{
	    copy_slice(buffer, &weapon_image, (int32)texture_x, dest_x, weapon_upper_top,
		       (int32)weapon_sprite_size.y, 0);
	    texture_x += texture_mapper;
	}
    }
}
