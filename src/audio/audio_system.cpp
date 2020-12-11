#include "audio/audio_system.h"
#include "common/atomics.h"
#include "audio/midi_system.h"
#include "audio/synth_system.hpp"
#include "common/profiler.hpp"

#include <sokol/sokol_audio.h>
#include "ui/imgui.h"

static u32 ms_tick;

PIM_C_BEGIN

static void audio_main(float* buffer, i32 ticks, i32 num_channels)
{
    for (i32 i = 0; i < ticks; ++i)
    {
        buffer[i * 2 + 0] = 0.0f;
        buffer[i * 2 + 1] = 0.0f;
    }
    SynthSys::OnSample(buffer, ticks, num_channels);
    fetch_add_u32(&ms_tick, (u32)ticks, MO_Release);
}

void audio_sys_init(void)
{
    ms_tick = 0;

    saudio_desc desc = {};
    desc.num_channels = 2;
    desc.sample_rate = 44100;
    desc.stream_cb = audio_main;
    saudio_setup(&desc);

    midi_sys_init();
    SynthSys::Init();
}

ProfileMark(pm_update, audio_sys_update)
void audio_sys_update(void)
{
    ProfileScope(pm_update);
    midi_sys_update();
    SynthSys::Update();
}

void audio_sys_shutdown(void)
{
    SynthSys::Shutdown();
    midi_sys_shutdown();
    saudio_shutdown();
}

ProfileMark(pm_ongui, audio_sys_ongui)
void audio_sys_ongui(bool* pEnabled)
{
    ProfileScope(pm_ongui);
    if (ImGui::Begin("Audio", pEnabled))
    {
        SynthSys::OnGui();
    }
    ImGui::End();
}

u32 audio_sys_tick(void)
{
    return load_u32(&ms_tick, MO_Relaxed);
}

PIM_C_END
