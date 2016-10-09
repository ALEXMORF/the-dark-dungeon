#include "game_audio.h"

void Audio_Task_List::push_task(Loaded_Audio *loaded_audio)
{
    assert(length < AUDIO_TASK_MAX);
    
    content[length].loaded_audio = loaded_audio;
    content[length].current_position = 0;
    content[length].is_finished = false;
    ++length;
}

void Audio_Task_List::push_task_looped(Loaded_Audio *loaded_audio)
{
    push_task(loaded_audio);
    content[length-1].is_looping = true;
}

void Audio_Task_List::remove_task(int index)
{
    assert(index >= 0 && index < AUDIO_TASK_MAX);
    assert(length > 0);
    
    content[index] = content[--length];
}

