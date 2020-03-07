#include "audio_system.h"
#include "components/system.h"

#include <sokol/sokol_audio.h>

namespace AudioSystem
{
    static void AudioMain(f32* buffer, i32 num_frames, i32)
    {
        for (i32 i = 0; i < num_frames; ++i)
        {
            *buffer++ = 0.0f;
            *buffer++ = 0.0f;
        }
    }

    static void Init()
    {
        saudio_desc desc = {};
        desc.num_channels = 2;
        desc.sample_rate = 44100;
        desc.stream_cb = AudioMain;
        saudio_setup(&desc);
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        saudio_shutdown();
    }

    static System ms_system
    {
        "AudioSystem",
        {},
        Init,
        Update,
        Shutdown,
    };
};
