#include "game_render.h"

inline uint32
darken(uint32 original)
{
    return (original >> 1) & 8355711;
}

internal void
fill_buffer(Game_Offscreen_Buffer *buffer, uint32 color)
{
    uint32 *pixel = (uint32 *)buffer->memory;
    int32 pixel_count = buffer->width * buffer->height;
    for (int i = 0; i < pixel_count; ++i)
    {
        pixel[i] = color;
    }
}

internal void
draw_line(Game_Offscreen_Buffer *buffer, int32 x0, int32 y0, int32 x1, int32 y1, uint32 color)
{
    uint32 *pixels = (uint32 *)buffer->memory;
    
    real32 slope = (real32)(y1 - y0)/(x1 - x0);
    if (abs(slope) > 1.0f)
    {
        if (y0 > y1)
        {
            swap(x0, x1);
            swap(y0, y1);
        }
        slope = 1.0f / slope;
        for (int32 y = y0; y <= y1; ++y)
        {
            int32 x = (int32)(x0 + slope*(y - y0));
            if (x >= 0 && x < buffer->width && y >= 0 && y < buffer->height)
            {
                pixels[buffer->width * y + x] = color;
            }
        }
    }
    else
    {
        if (x0 > x1)
        {
            swap(x0, x1);
            swap(y0, y1);
        }
        for (int32 x = x0; x <= x1; ++x)
        {
            int32 y = (int32)(y0 + slope*(x - x0));
            if (x >= 0 && x < buffer->width && y >= 0 && y < buffer->height) 
            {
                pixels[buffer->width * y + x] = color;
            }
        }
    }
}

internal void
draw_rectangle(Game_Offscreen_Buffer *buffer,
               int32 min_x, int32 min_y,
               int32 max_x, int32 max_y,
               uint32 color)
{
    if_do(min_x < 0, min_x = 0);
    if_do(min_y < 0, min_y = 0);
    if_do(max_x > buffer->width, max_x = buffer->width);
    if_do(max_y > buffer->height, max_y = buffer->height);

    int32 bytes_per_pixel = 4;
    
    uint8 *row = (uint8 *)buffer->memory;
    row += bytes_per_pixel*min_x + buffer->pitch*min_y;

    for (int32 y = min_y; y < max_y; ++y)
    {
        uint32 *pixel = (uint32 *)row;
        for (int32 x = min_x; x < max_x; ++x)
        {
            *pixel++ = color;
        }
        row += buffer->pitch;
    }
}

internal void
draw_rectangle(Game_Offscreen_Buffer *buffer,
               real32 min_x, real32 min_y,
               real32 max_x, real32 max_y,
               uint32 color)
{
    draw_rectangle(buffer, (int32)min_x, (int32)min_y, (int32)max_x, (int32)max_y, color);
}

internal void
copy_slice(Game_Offscreen_Buffer *buffer, Loaded_Image *loaded_image,
           int32 source_x, int32 dest_x, int32 dest_y, int32 dest_height,
           Shader_Fn *shader = 0, real32 shader_ratio = 0.5f)
{
    if (dest_x >= 0 && dest_x < buffer->width)
    {
        int32 bytes_per_pixel = 4;
        uint32 *source_pixel = (uint32 *)loaded_image->data + source_x;
        
        real32 mapper = (real32)loaded_image->height / (real32)dest_height;
        real32 source_y = 0.0f;
        
        //NOTE(chen): clip the size and move the source pointer to where it should be 
        uint32 *dest_pixel = (uint32 *)buffer->memory + dest_x + dest_y * buffer->width;
        if (dest_y < 0)
        {
            dest_pixel += -dest_y * buffer->width;
            source_y += mapper * -dest_y;
            dest_y = 0;
            dest_height = buffer->height;
        }
        if (dest_height > buffer->height)
        {
            dest_height = buffer->height;
        }
        if (dest_y + dest_height > buffer->height)
        {
            dest_height = buffer->height - dest_y;
        }
        
        //render
        for (int32 y = 0; y < dest_height; ++y)
        {
            uint32 source_value = *(uint32 *)((uint8 *)source_pixel + ((int32)source_y * loaded_image->pitch));
            if ((source_value & alpha_mask) != 0)
            {
                if (shader)
                {
                    *dest_pixel = shader(source_value);
                }
                else
                {
                    *dest_pixel = source_value;
                }
            }
            source_y += mapper;

            dest_pixel += buffer->width;
        }
    }
}

internal void
draw_rectangle(Game_Offscreen_Buffer *buffer, Loaded_Image *bitmap, int32 min_x, int32 min_y,
               int32 max_x, int32 max_y)
{
    if_do(min_x < 0, min_x = 0);
    if_do(min_y < 0, min_y = 0);
    if_do(max_x > buffer->width, max_x = buffer->width);
    if_do(max_y > buffer->height, max_y = buffer->height);

    
}
    
internal void
draw_bitmap(Game_Offscreen_Buffer *buffer, Loaded_Image *bitmap, int32 min_x, int32 min_y,
            int32 max_x, int32 max_y)
{
    if_do(min_x < 0, min_x = 0);
    if_do(min_y < 0, min_y = 0);
    if_do(max_x > buffer->width, max_x = buffer->width);
    if_do(max_y > buffer->height, max_y = buffer->height);
    
    real32 image_height = (real32)max_y - (real32)min_y;
    real32 image_width = (real32)max_x - (real32)min_x;
    
    real32 image_to_bitmap_x = (real32)bitmap->width / image_width;
    real32 image_to_bitmap_y = (real32)bitmap->height / image_height;
    
    uint32 *dest_ptr = (uint32 *)buffer->memory + min_x + min_y * buffer->width;
    uint8 *src_ptr = bitmap->data;
    
    real32 bitmap_y = 0.0f;
    for (int y = min_y; y < max_y; ++y)
    {
        uint32 *dest_ptr_w = (uint32 *)dest_ptr;

        real32 bitmap_x = 0.0f;
        for (int x = min_x; x < max_x; ++x)
        {
            uint8 *current_src_ptr = src_ptr + (int32)floorf(bitmap_x) * bitmap->bytes_per_pixel + (int32)floorf(bitmap_y) * bitmap->pitch;
            
            uint32 dest_value = *dest_ptr_w;
            uint32 src_value = *(uint32 *)current_src_ptr;
            
            uint8 old_R = (uint8)((dest_value & red_mask) >> 16);
            uint8 old_G = (uint8)((dest_value & green_mask) >> 8);
            uint8 old_B = (uint8)((dest_value & blue_mask) >> 0);
            
            uint8 new_A = (uint8)((src_value & alpha_mask) >> 24);
            uint8 new_R = (uint8)((src_value & red_mask) >> 16);
            uint8 new_G = (uint8)((src_value & green_mask) >> 8);
            uint8 new_B = (uint8)((src_value & blue_mask) >> 0);
            
            real32 A = (real32)new_A / 255.0f;

            uint8 R = (uint8)((1.0f - A) * (real32)old_R + A * (real32)new_R);
            uint8 G = (uint8)((1.0f - A) * (real32)old_G + A * (real32)new_G);
            uint8 B = (uint8)((1.0f - A) * (real32)old_B + A * (real32)new_B);
            
            *dest_ptr_w++ = (uint32)((R << 16) | (G << 8) | B);

            bitmap_x += image_to_bitmap_x;
        }
        
        dest_ptr += buffer->width;

        bitmap_y += image_to_bitmap_y;
    }
}
