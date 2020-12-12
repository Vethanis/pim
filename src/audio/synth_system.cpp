#include "audio/synth_system.hpp"
#include "ui/imgui.h"
#include "common/profiler.hpp"
#include "input/input_system.h"
#include "math/types.h"
#include "math/scalar.h"
#include "math/float4.hpp"
#include "audio/modular/board.hpp"

static constexpr double kSamplesPerSecond = 44100.0;
static constexpr double kSecondsPerSample = 1.0 / kSamplesPerSecond;

static Modular::Board ms_board;
static float ms_volume = 0.0f;
static bool ms_capture;
static double ms_freq;
static i32 ms_octave = 4;
static i32 ms_note = 0;

static double NoteToHertz(double note)
{
    double a = (note - 69.0) * (1.0 / 12.0);
    return 440.0f * exp2(a);
}

static double HertzToDeltaPhase(double hz)
{
    return hz * (1.0 / 44100.0);
}

static double SineWave(double phase)
{
    return sin(phase * kTau);
}

static double Impulse(double phase, double rcpScale)
{
    double t = phase * rcpScale;
    return t * exp(1.0 - t);
}

struct Oscillator
{
    double m_phase;
    double m_dphase;
    i32 m_octave;
    i32 m_semitone;
    float m_fine;
    double m_offset;

    void OnGui()
    {
        ImGui::PushID(this);
        ImGui::SliderInt("Octave", &m_octave, -4, 4);
        ImGui::SliderInt("Semitone", &m_semitone, -12, 12);
        ImGui::SliderFloat("Fine", &m_fine, -1.0f, 1.0f);
        ImGui::PopID();
        double note = m_octave * 12 + m_semitone + m_fine;
        double hz = NoteToHertz(note);
        m_offset = hz;
    }
    void SetFreq(double hz)
    {
        m_dphase = HertzToDeltaPhase(hz + m_offset);
    }
    double Sample(double phaseMod)
    {
        m_phase = fmod(m_phase + m_dphase + phaseMod, 1.0f);
        return SineWave(m_phase);
    }
};

struct Envelope
{
    double m_phase;
    double m_rcpScale;

    void OnGui()
    {
        ImGui::PushID(this);
        float scale = 1.0f / f1_max(kEpsilon, (float)m_rcpScale);
        ImGui::SliderFloat("Scale", &scale, kMilli, 1.0f);
        m_rcpScale = 1.0 / f1_max(kEpsilon, scale);
        ImGui::PopID();
    }
    void Trigger()
    {
        m_phase = 0.0;
    }
    double Sample()
    {
        double value = Impulse(m_phase, m_rcpScale);
        m_phase += kSecondsPerSample;
        return value;
    }
};

struct Voice
{
    Oscillator m_oscillator;
    Envelope m_envelope;
    float m_volume;

    void OnGui()
    {
        ImGui::PushID(this);
        ImGui::SliderFloat("Volume", &m_volume, 0.0f, 1.0f);
        m_oscillator.OnGui();
        m_envelope.OnGui();
        ImGui::PopID();
    }
    void SetVolume(float volume)
    {
        m_volume = volume;
    }
    double Sample(double phaseMod)
    {
        double value = m_volume * m_oscillator.Sample(phaseMod);
        double envelope = m_envelope.Sample();
        return value * envelope;
    }
};

static Voice ms_mod;
static Voice ms_primary;

void SynthSys::Init()
{

}

ProfileMark(pm_update, SynthSys::Update)
void SynthSys::Update()
{
    ProfileScope(pm_update);

    //if (ms_capture)
    {
        bool gate = false;
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
                gate = true;
            }
        }
        ms_freq = NoteToHertz(ms_octave * 12.0 + (double)ms_note);
        ms_primary.m_oscillator.SetFreq(ms_freq);
        ms_mod.m_oscillator.SetFreq(ms_freq);
        if (gate)
        {
            ms_primary.m_envelope.Trigger();
            ms_mod.m_envelope.Trigger();
        }
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
        ms_primary.OnGui();
        ms_mod.OnGui();
        ImGui::TreePop();
    }
}

void SynthSys::OnSample(float *const pim_noalias buffer, i32 frames, i32 channels)
{
    ASSERT(channels == 2);
    const double volume = ms_volume;
    for (i32 i = 0; i < frames; ++i)
    {
        double pm = ms_mod.Sample(0.0);
        double value = ms_primary.Sample(pm);
        value *= volume;
        float value32 = (float)value;
        buffer[i * 2 + 0] = value32;
        buffer[i * 2 + 1] = value32;
    }
}
