#include "audio/audio_system.h"
#include "common/atomics.h"
#include "audio/midi_system.h"
#include "common/profiler.h"

#include <sokol/sokol_audio.h>
#include <ui/cimgui_ext.h>

static u32 ms_tick;

static void audio_main(float* buffer, i32 ticks, i32 num_channels)
{
    for (i32 i = 0; i < ticks; ++i)
    {
        buffer[i * 2 + 0] = 0.0f;
        buffer[i * 2 + 1] = 0.0f;
    }
    fetch_add_u32(&ms_tick, (u32)ticks, MO_Release);
}

void AudioSys_Init(void)
{
    ms_tick = 0;

    const saudio_desc desc = {
        .num_channels = 2,
        .sample_rate = 44100,
        .stream_cb = audio_main,
    };
    saudio_setup(&desc);

    MidiSys_Init();
}

ProfileMark(pm_update, AudioSys_Update)
void AudioSys_Update(void)
{
    ProfileBegin(pm_update);

    MidiSys_Update();

    ProfileEnd(pm_update);
}

void AudioSys_Shutdown(void)
{
    MidiSys_Shutdown();
    saudio_shutdown();
}

ProfileMark(pm_ongui, AudioSys_Gui)
void AudioSys_Gui(bool* pEnabled)
{

}

u32 AudioSys_Ticks(void)
{
    return load_u32(&ms_tick, MO_Relaxed);
}
