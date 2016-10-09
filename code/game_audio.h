#pragma once

#define AUDIO_TASK_MAX 100

struct Audio_Task
{
    Loaded_Audio *loaded_audio;
    int32 current_position;
    bool is_finished;
    bool is_looping;
};

struct Audio_Task_List
{
    Audio_Task content[AUDIO_TASK_MAX];
    int32 length;

    void push_task(Loaded_Audio *audio);
    void push_task_looped(Loaded_Audio *audio);
    void remove_task(int index);
};
