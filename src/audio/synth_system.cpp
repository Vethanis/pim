#include "audio/synth_system.hpp"
#include "ui/imgui.h"
#include "common/profiler.hpp"
#include "input/input_system.h"
#include "math/types.h"
#include "math/scalar.h"
#include "math/float4.hpp"
#include "audio/modular/board.hpp"

static Modular::Board ms_board;
static float ms_volume = 0.0f;
static bool ms_capture;
static float ms_freq;
static float ms_dphase;
static float ms_phase;
static i32 ms_octave = 4;
static i32 ms_note = 0;

inline float MidiToHertz(i32 note)
{
    float a = (note - 69) * (1.0f / 12.0f);
    return 440.0f * exp2f(a);
}

inline float HertzToDeltaPhase(float hz)
{
    return hz * (1.0f / 44100.0f);
}

void SynthSys::Init()
{

}

ProfileMark(pm_update, SynthSys::Update)
void SynthSys::Update()
{
    ProfileScope(pm_update);

    //if (ms_capture)
    {
        if (input_keydown(KeyCode_Z))
        {
            --ms_octave;
        }
        if (input_keydown(KeyCode_X))
        {
            ++ms_octave;
        }
        for (i32 i = 1; i <= 9; ++i)
        {
            if (input_keydown((KeyCode)(KeyCode_0 + i)))
            {
                ms_note = i;
            }
        }
        ms_freq = MidiToHertz(ms_octave * 12 + ms_note);
        ms_dphase = HertzToDeltaPhase(ms_freq);
    }
}

void SynthSys::Shutdown()
{

}

ProfileMark(pm_ongui, SynthSys::OnGui)
void SynthSys::OnGui()
{
    ProfileScope(pm_ongui);
    if (ImGui::TreeNodeEx("Synth", ImGuiTreeNodeFlags_Framed))
    {
        ImGui::Checkbox("Use Keyboard", &ms_capture);
        ImGui::SliderFloat("Volume", &ms_volume, 0.0f, 1.0f);
        ImGui::TreePop();
    }
}

void SynthSys::OnSample(float *const pim_noalias buffer, i32 frames, i32 channels)
{
    ASSERT(channels == 2);
    const i32 spp = 8;
    const float dphase = ms_dphase / spp;
    const float volume = ms_volume;
    float phase = ms_phase;
    for (i32 i = 0; i < frames; ++i)
    {
        float value = 0.0f;
        for (i32 j = 0; j < spp; ++j)
        {
            phase = fmodf(phase + dphase, 1.0f);
            value += sinf(phase * kTau);
        }
        value *= volume / spp;
        buffer[i * 2 + 0] = value;
        buffer[i * 2 + 1] = value;
    }
    ms_phase = phase;
}
