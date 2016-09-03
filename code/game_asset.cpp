#include "game_asset.h"

inline Loaded_Image
load_image(Platform_Load_Image *platform_load_image, char *filename)
{
    Loaded_Image result = {};
    
    int32 bytes_per_pixel = 4;
    result.data = platform_load_image(filename, &result.width, &result.height, 0, bytes_per_pixel);
    result.bytes_per_pixel = bytes_per_pixel;

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
    
    return result;
}

inline void
free_image(Platform_Free_Image *platform_free_image, Loaded_Image *loaded_image)
{
    platform_free_image(loaded_image->data);
}

