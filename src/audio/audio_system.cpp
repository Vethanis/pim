#include "audio_system.hpp"
#include "audio/synth_system.hpp"
#include "common/profiler.hpp"
#include <sokol/sokol_audio.h>
#include "ui/imgui.h"

static void audio_main(float* buffer, i32 num_frames, i32 num_channels)
{
    for (i32 i = 0; i < num_frames; ++i)
    {
        buffer[i * 2 + 0] = 0.0f;
        buffer[i * 2 + 1] = 0.0f;
    }
    SynthSys::OnSample(buffer, num_frames, num_channels);
}

void AudioSys::Init()
{
    saudio_desc desc = {};
    desc.num_channels = 2;
    desc.sample_rate = 44100;
    desc.stream_cb = audio_main;
    saudio_setup(&desc);

    SynthSys::Init();
}

ProfileMark(pm_update, AudioSys::Update)
void AudioSys::Update()
{
    ProfileScope(pm_update);
    SynthSys::Update();
}

void AudioSys::Shutdown()
{
    SynthSys::Shutdown();
    saudio_shutdown();
}

ProfileMark(pm_ongui, AudioSys::OnGui)
void AudioSys::OnGui(bool* pEnabled)
{
    ProfileScope(pm_ongui);
    if (ImGui::Begin("Audio", pEnabled))
    {
        SynthSys::OnGui();
    }
    ImGui::End();
}
