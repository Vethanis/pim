#include "audio/synth_system.hpp"
#include <imgui/imgui.h>
#include "common/profiler.hpp"
#include "input/input_system.h"
#include "math/types.h"
#include "math/scalar.h"
#include "math/float4.hpp"
#include "audio/modular/board.hpp"
#include "common/random.h"

static constexpr float kSamplesPerSecond = 44100.0f;
static constexpr float kSecondsPerSample = 1.0f / kSamplesPerSecond;

static float AvoidZero(float x)
{
    float s = (x >= 0.0f) ? 1.0f : -1.0f;
    return s * f1_max(kEpsilon, f1_abs(x));
}

static float NoteToHertz(float note)
{
    float a = (note - 69.0f) * (1.0f / 12.0f);
    return 440.0f * exp2f(a);
}

static float HertzToDeltaPhase(float hz)
{
    return hz * (1.0f / 44100.0f);
}

static float SineWave(float phase)
{
    return sinf(phase * kTau);
}

static float Impulse(float phase, float scale)
{
    float t = phase * scale;
    return t * expf(1.0f - t);
}

static float RandomizeFloat(float value, float amount, float lo, float hi, prng_t& rng)
{
    float slider = f1_lerp(lo, hi, prng_f32(&rng));
    return f1_lerp(value, slider, amount);
}

static float RandomizeLog2(float value, float amount, float lo, float hi, prng_t& rng)
{
    float slider = exp2f(AvoidZero(f1_lerp(lo, hi, prng_f32(&rng))));
    return f1_lerp(value, slider, amount);
}

static i32 RandomizeInt(i32 value, float amount, i32 lo, i32 hi, prng_t& rng)
{
    float slider = f1_lerp((float)lo, (float)hi, prng_f32(&rng));
    return (i32)f1_round(f1_lerp((float)value, slider, amount));
}

static float RandomizeUnorm(float value, float amount, prng_t& rng)
{
    return RandomizeFloat(value, amount, 0.0f, 1.0f, rng);
}

static float RandomizeSnorm(float value, float amount, prng_t& rng)
{
    return RandomizeFloat(value, amount, -1.0f, 1.0f, rng);
}

static float RandomizeVolume(float value, float amount, prng_t& rng)
{
    return RandomizeLog2(value, amount, -10.0f, -1.0f, rng);
}

namespace ImGui
{
    static bool Slider(const char* label, i32* v, i32 lo, i32 hi)
    {
        return SliderInt(label, v, lo, hi);
    }
    static bool Slider(const char* label, float* v, float lo, float hi)
    {
        float value = (float)*v;
        bool changed = ImGui::SliderFloat(label, &value, lo, hi);
        *v = value;
        return changed;
    }
    static bool SliderLog2(const char* label, float* v, float lo, float hi)
    {
        float v2 = log2f(AvoidZero(*v));
        bool changed = Slider(label, &v2, lo, hi);
        *v = exp2f(AvoidZero(v2));
        return changed;
    }
    static bool SliderVolume(const char* label, float* v)
    {
        return SliderLog2(label, v, -10.0f, -1.0f);
    }
};

struct Oscillator
{
    float m_phase;
    float m_dphase;
    i32 m_octave;
    i32 m_semitone;
    float m_fine;
    float m_note;
    float m_randAmount;

    Oscillator() :
        m_phase(0.0f),
        m_dphase(0.0f),
        m_octave(0),
        m_semitone(0),
        m_fine(0.0f),
        m_randAmount(1.0f)
    {}

    void OnGui()
    {
        ImGui::PushID(this);
        ImGui::Indent();
        ImGui::Text("Oscillator");

        ImGui::Slider("Octave", &m_octave, -4, 4);
        ImGui::Slider("Semitone", &m_semitone, -12, 12);
        ImGui::Slider("Fine", &m_fine, -1.0f, 1.0f);
        ImGui::SliderLog2("Random Amount", &m_randAmount, -10.0f, -1.0f);
        if (ImGui::Button("Randomize"))
        {
            prng_t rng = prng_get();
            Randomize(rng, 1.0f);
            prng_set(rng);
        }

        ImGui::Unindent();
        ImGui::PopID();
    }
    void Randomize(prng_t& rng, float amount)
    {
        amount *= m_randAmount;
        m_phase = RandomizeUnorm(m_phase, amount, rng);
        m_octave = RandomizeInt(m_octave, amount, -4, 4, rng);
        m_semitone = RandomizeInt(m_semitone, amount, -12, 12, rng);
        m_fine = RandomizeSnorm(m_fine, amount, rng);
        m_dphase = HertzToDeltaPhase(NoteToHertz(m_note + m_octave * 12 + m_semitone + m_fine));
    }
    void OnNote(float note)
    {
        m_note = note;
        m_dphase = HertzToDeltaPhase(NoteToHertz(m_note + m_octave * 12 + m_semitone + m_fine));
    }
    void OnGate(float gate)
    {

    }
    float Sample(float phaseMod)
    {
        m_phase = fmodf(m_phase + m_dphase + phaseMod, 1.0f);
        return SineWave(m_phase);
    }
};

struct Envelope
{
    float m_phase;
    float m_scale;
    float m_value;
    float m_randAmount;

    Envelope() :
        m_phase(0.0f),
        m_scale(1.0f),
        m_value(0.0f),
        m_randAmount(1.0f)
    {}
    void OnGui()
    {
        ImGui::PushID(this);
        ImGui::Indent();
        ImGui::Text("Envelope");

        ImGui::SliderLog2("Scale", &m_scale, -10.0f, 10.0f);
        ImGui::SliderLog2("Random Amount", &m_randAmount, -10.0f, -1.0f);
        if (ImGui::Button("Randomize"))
        {
            prng_t rng = prng_get();
            Randomize(rng, 1.0f);
            prng_set(rng);
        }

        ImGui::Unindent();
        ImGui::PopID();
    }
    void Randomize(prng_t& rng, float amount)
    {
        amount *= m_randAmount;
        m_scale = RandomizeLog2(m_scale, amount, -10.0f, 10.0f, rng);
    }
    void OnNote(float note)
    {

    }
    void OnGate(float gate)
    {
        if (gate > 0.0f)
        {
            m_phase = 0.0f;
        }
    }
    float Sample()
    {
        float value = Impulse(m_phase, m_scale);
        m_phase += kSecondsPerSample;
        m_value = f1_lerp(m_value, value, 0.25f);
        return m_value;
    }
};

struct Voice
{
    float m_volume;
    float m_randAmount;
    Oscillator m_oscillator;
    Envelope m_envelope;

    Voice() :
        m_volume(0.5f),
        m_randAmount(1.0f)
    {}
    void SetVolume(float volume)
    {
        m_volume = volume;
    }

    void OnGui()
    {
        ImGui::PushID(this);
        ImGui::Indent();
        ImGui::Text("Voice");

        ImGui::SliderVolume("Volume", &m_volume);
        ImGui::SliderLog2("Random Amount", &m_randAmount, -10.0f, -1.0f);
        if (ImGui::Button("Randomize"))
        {
            prng_t rng = prng_get();
            Randomize(rng, 1.0f);
            prng_set(rng);
        }
        ImGui::Separator();
        m_oscillator.OnGui();
        m_envelope.OnGui();

        ImGui::Unindent();
        ImGui::PopID();
    }
    void Randomize(prng_t& rng, float amount)
    {
        amount *= m_randAmount;
        m_oscillator.Randomize(rng, amount);
        m_envelope.Randomize(rng, amount);
        m_volume = RandomizeVolume(m_volume, amount, rng);
    }
    void OnNote(float note)
    {
        m_oscillator.OnNote(note);
        m_envelope.OnNote(note);
    }
    void OnGate(float gate)
    {
        m_oscillator.OnGate(gate);
        m_envelope.OnGate(gate);
    }
    float Sample(float phaseMod)
    {
        return m_oscillator.Sample(phaseMod) * m_envelope.Sample() * m_volume;
    }
};

struct Synth
{
    float m_volume;
    float m_randAmount;
    Voice m_primary;
    Voice m_mods[16];

    Synth() :
        m_volume(0.0f),
        m_randAmount(0.5f)
    {
        for (Voice& m : m_mods)
        {
            m.SetVolume(kEpsilon);
        }
    }

    void OnGui()
    {
        ImGui::PushID(this);
        ImGui::Indent();
        ImGui::Text("Synth");

        ImGui::SliderVolume("Volume", &m_volume);
        ImGui::SliderLog2("Random Amount", &m_randAmount, -10.0f, -1.0f);
        if (ImGui::Button("Randomize"))
        {
            prng_t rng = prng_get();
            Randomize(rng, 1.0f);
            prng_set(rng);
        }
        ImGui::Separator();
        m_primary.OnGui();
        ImGui::Separator();
        for (Voice& m : m_mods)
        {
            m.OnGui();
            ImGui::Separator();
        }

        ImGui::Unindent();
        ImGui::PopID();
    }
    void Randomize(prng_t& rng, float amount)
    {
        amount *= m_randAmount;
        m_primary.Randomize(rng, amount);
        for (Voice& m : m_mods)
        {
            m.Randomize(rng, amount);
        }
    }
    void OnNote(float note)
    {
        m_primary.OnNote(note);
        for (Voice& m : m_mods)
        {
            m.OnNote(note);
        }
    }
    void OnGate(float gate)
    {
        m_primary.OnGate(gate);
        for (Voice& m : m_mods)
        {
            m.OnGate(gate);
        }
    }
    void Sample(float* buffer, i32 length)
    {
        const float modScale = 1.0f / NELEM(m_mods);
        const float volume = m_volume;
        for (i32 i = 0; i < length; ++i)
        {
            float mod = 0.0f;
            for (Voice& m : m_mods)
            {
                mod += m.Sample(0.0f);
            }
            float value = m_primary.Sample(mod * modScale) * volume;

            float val32 = (float)value;
            buffer[i * 2 + 0] = val32;
            buffer[i * 2 + 1] = val32;
        }
    }
};

static Synth ms_synth;

void SynthSys::Init()
{
    ms_synth = {};
}

ProfileMark(pm_update, SynthSys::Update)
void SynthSys::Update()
{
    ProfileScope(pm_update);

    //if (ms_capture)
    {
        static i32 s_octave = 4;
        static i32 s_note = 0;

        float gate = 0.0f;
        if (input_keydown(KeyCode_Z))
        {
            --s_octave;
        }
        if (input_keydown(KeyCode_X))
        {
            ++s_octave;
        }
        for (i32 i = 1; i <= 9; ++i)
        {
            if (input_keydown((KeyCode)(KeyCode_0 + i)))
            {
                s_note = i;
                gate = 1.0;
            }
        }
        float note = s_octave * 12.0f + s_note;
        ms_synth.OnNote(note);
        ms_synth.OnGate(gate);
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
        ms_synth.OnGui();
        ImGui::TreePop();
    }
}

void SynthSys::OnSample(float* buffer, i32 frames, i32 channels)
{
    ASSERT(channels == 2);
    ms_synth.Sample(buffer, frames);
}
