#include "game_asset.h"

inline Loaded_Image
load_image(Platform_Load_Image *platform_load_image, char *filename)
{
    Loaded_Image result = {};
    result.data = platform_load_image(filename,
				      &result.width, &result.height, &result.bytes_per_pixel, 0);
    return result;
}

inline void
free_image(Platform_Free_Image *platform_free_image, Loaded_Image *loaded_image)
{
    platform_free_image(loaded_image->data);
}
