#include "game_ui.h"

inline int
get_string_length(char *str)
{
    int result = 0;
    while (*str++)
    {
        result += 1;
    }
    return result;
}

inline int
get_font_index(char character)
{
    if (character >= 'a' && character <= 'z')
    {
        character += 'A' - 'a';
    }
    assert(character >= '(' && character <= 'Z');
    
    char initial_character = '(';
    int start_index_offset = 8;
    
    return start_index_offset + (character - initial_character);
}

#define draw_string(str_drawer, min_x, min_y, max_x, max_y, string_format, ...) { char string[255] = {}; snprintf(string, sizeof(string), string_format, ##__VA_ARGS__); draw_string_unformatted(str_drawer, min_x, min_y, max_x, max_y, string); } 
#define draw_string_autosized(str_drawer, min_x, min_y, font_width, font_height, string_format, ...) { char string[255] = {}; snprintf(string, sizeof(string), string_format, ##__VA_ARGS__); draw_string_unformatted(str_drawer, min_x, min_y, min_x + font_width * get_string_length(string), min_y + font_height, string); } 
internal void
draw_string_unformatted(String_Drawer *drawer, int min_x, int min_y, int max_x, int max_y,
                        char *string)
{
    Game_Offscreen_Buffer *buffer = drawer->buffer;
    
    if_do(min_x < 0, min_x = 0);
    if_do(min_y < 0, min_y = 0);
    if_do(max_x > buffer->width, max_x = buffer->width);
    if_do(max_y > buffer->height, max_y = buffer->height);

    int string_length = get_string_length(string);
    int bitmap_width = (int32)(((real32)max_x - min_x) / string_length);
    
    for (int i = 0; i < string_length; ++i)
    {
        if (string[i] == ' ')
        {
            continue;
        }
        
        int bitmap_index = get_font_index(string[i]);
        int bitmap_min_x = min_x + bitmap_width * i;

        Loaded_Image font_image = extract_image_from_sheet(drawer->font_sheet, bitmap_index, 0);
        draw_bitmap(drawer->buffer, &font_image, bitmap_min_x, min_y, bitmap_min_x + bitmap_width, max_y);
    }
}

#define print(str, ...) draw_string_autosized(&str_drawer, layout_x, layout_height, font_size, font_size, str, ##__VA_ARGS__); layout_height += layout_dheight;
