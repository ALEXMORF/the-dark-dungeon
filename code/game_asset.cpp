#include "game_asset.h"

//////////////////////////////////////////////////
// Primary functions
//////////////////////////////////////////////////

inline Loaded_Image
load_image(Platform_Load_Image *platform_load_image, char *filename)
{
    Loaded_Image result = {};
    
    int32 bytes_per_pixel = 4;
    result.data = platform_load_image(filename, &result.width, &result.height, 0, bytes_per_pixel);
    result.bytes_per_pixel = bytes_per_pixel;
    result.pitch = bytes_per_pixel * result.width;
    /*NOTE(chen):
      us: AA RR GG BB
      stb: AA BB GG RR

      Flip them back!
    */
    uint8 *pixel_components = (uint8 *)result.data;    
    for (int32 i = 0; i < result.width * result.height; ++i)
    {
        swap(pixel_components[0], pixel_components[2]);
        pixel_components += bytes_per_pixel;
    }

    assert(result.data);
    return result;
}

inline Loaded_Image_Sheet
load_image_sheet(Platform_Load_Image *platform_load_image, char *filename)
{
    Loaded_Image_Sheet result = {};
    
    int32 bytes_per_pixel = 4;
    result.data = platform_load_image(filename, &result.width, &result.height, 0, bytes_per_pixel);
    result.bytes_per_pixel = bytes_per_pixel;
    result.pitch = bytes_per_pixel * result.width;
    /*NOTE(chen):
      us: AA RR GG BB
      stb: AA BB GG RR

      Flip them back!
    */
    uint8 *pixel_components = (uint8 *)result.data;    
    for (int32 i = 0; i < result.width * result.height; ++i)
    {
        swap(pixel_components[0], pixel_components[2]);
        pixel_components += bytes_per_pixel;
    }

    assert(result.data);
    return result;
}

inline void
free_image(Platform_Free_Image *platform_free_image, Loaded_Image *loaded_image)
{
    platform_free_image(loaded_image->data);
}

inline void
free_image_sheet(Platform_Free_Image *platform_free_image, Loaded_Image_Sheet *loaded_image_sheet)
{
    platform_free_image(loaded_image_sheet->data);
}

inline Loaded_Image
extract_image_from_sheet(Loaded_Image_Sheet *sheet, int32 image_x, int32 image_y)
{
    assert(image_x >= 0 && image_x < sheet->image_count_x);
    assert(image_y >= 0 && image_y < sheet->image_count_y);
    
    int32 x_offset = image_x * (sheet->image_width + sheet->stride_offset);
    int32 y_offset = image_y * (sheet->image_height + sheet->stride_offset);
    
    Loaded_Image result = {};
    result.data = sheet->data + (x_offset * sheet->bytes_per_pixel) + (y_offset * sheet->pitch);
    result.width = sheet->image_width;
    result.height = sheet->image_height;
    result.bytes_per_pixel = sheet->bytes_per_pixel;
    result.pitch = sheet->pitch;
        
    return result;
}

inline Loaded_Image
extract_image_from_sheet(Loaded_Image_Sheet *sheet, v2i image_index)
{
    return extract_image_from_sheet(sheet, image_index.x, image_index.y);
}

inline void
config_image_sheet(Loaded_Image_Sheet *sheet, int32 image_count_x, int32 image_count_y,
                   uint32 stride_offset, int32 image_width, int32 image_height)
{
    sheet->image_count_x = image_count_x;
    sheet->image_count_y = image_count_y;
    sheet->stride_offset = stride_offset;
    sheet->image_width = image_width;
    sheet->image_height = image_height;
}

//NOTE(chen): samething as config, but automatically deduce each image dimension
//            stride_offset always assumed to be 0 
inline void
auto_config_image_sheet(Loaded_Image_Sheet *sheet, int32 image_count_x, int32 image_count_y)
{
    int32 stride_offset = 0;
    int32 image_width = sheet->width / image_count_x;
    int32 image_height = sheet->height / image_count_y;
    config_image_sheet(sheet, image_count_x, image_count_y, stride_offset, image_width, image_height);
}

inline Loaded_Audio
load_audio(Platform_Load_Audio *platform_load_audio, char *filename)
{
    Loaded_Audio result = {};
    result.memory = platform_load_audio(filename, &result.channels, &result.byte_per_sample, &result.byte_size);
    return result;
}

