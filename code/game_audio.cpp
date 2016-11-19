#include "game_audio.h"

void Audio_System::push_task(Loaded_Audio *loaded_audio, real32 volume)
{
    assert(length < AUDIO_TASK_MAX);
    
    content[length].loaded_audio = loaded_audio;
    content[length].current_position = 0;
    content[length].volume = volume;
    content[length].is_finished = false;
    ++length;
}

void Audio_System::push_task_looped(Loaded_Audio *loaded_audio, real32 volume)
{
    push_task(loaded_audio, volume);
    content[length-1].is_looping = true;
}

void Audio_System::remove_task(int index)
{
    assert(index >= 0 && index < AUDIO_TASK_MAX);
    assert(length > 0);
    
    content[index] = content[--length];
}

