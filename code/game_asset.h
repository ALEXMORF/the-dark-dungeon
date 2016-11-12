#pragma once

struct Loaded_Image
{
    uint8 *data;
    
    int32 width;
    int32 height;
    int32 bytes_per_pixel;

    int32 pitch;
};

struct Loaded_Image_Sheet
{
    //as a image spec
    uint8 *data;
    
    int32 width;
    int32 height;
    int32 bytes_per_pixel;
    
    int32 pitch;

    //as a sheet spec
    int32 image_count_x;
    int32 image_count_y;
    
    uint32 stride_offset;
    
    int32 image_width;
    int32 image_height;
};

struct Texture_List
{
    Loaded_Image *E;
    int32 count;
};

struct Loaded_Audio
{
    void *memory;
    int32 channels;
    int32 byte_per_sample;
    uint32 byte_size;
};
