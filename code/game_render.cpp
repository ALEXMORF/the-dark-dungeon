#include "game_render.h"

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

