#include "audio_system.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "math/math.h"
#include "common/find.h"
#include "systems/input_system.h"
#include "ui/imgui.h"
#include "common/random.h"

#include <sokol/sokol_audio.h>

namespace AudioSystem
{
    constexpr f32 SampleFreq = 44100.0f;
    constexpr f32 DeltaTime = 1.0f / SampleFreq;

    inline f32 NoteToFreq(f32 note)
    {
        return math::exp2((note - 69.0f) / 12.0f) * 440.0f;
    }

    inline f32 FreqToNote(f32 freq)
    {
        return (math::log2(freq / 440.0f) * 12.0f) + 69.0f;
    }

    inline float2 PhaseMod(float2 phase, float2 signal)
    {
        return math::frac(phase + signal * DeltaTime);
    }

    inline float2 SineOsc(float2& phase, float2 freq)
    {
        phase = PhaseMod(phase, freq);
        return math::sin(phase * math::Tau);
    }

    inline f32 ImpulseEnv(float2& env)
    {
        f32 h = env.y * env.x;
        env.x += DeltaTime;
        return math::saturate(h * math::exp(1.0f - h));
    }

    inline float2 Mix(float2 value, float2 volPan)
    {
        return math::exp2(volPan.x - 1.0f) * float2(1.0f - volPan.y, volPan.y) * value;
    }

    struct Voice
    {
        static constexpr i32 NumOscs = 24;
        static constexpr f32 Scale = 1.0f / (f32)NumOscs;

        f32 m_modAmt = 0.1f;
        i32 m_mainOffset = -12;
        f32 m_mainFreq;
        float2 m_mainPhase;
        float2 m_mainEnv = float2(10.0f, 15.0f);
        float2 m_mainMix = float2(-1.0f, 0.5f);

        f32 m_modFreqs[NumOscs];
        float2 m_modPhase[NumOscs];
        float2 m_modEnvs[NumOscs];
        float2 m_modMix[NumOscs];

        inline void Randomize()
        {
            for (i32 i = 0; i < NumOscs; ++i)
            {
                m_modPhase[i] = float2(Random::NextF32(), Random::NextF32());
                m_modEnvs[i] = float2(10.0f, Random::NextF32(1.0f, 40.0f));
                m_modMix[i] = float2(Random::NextF32(-5.0f, 0.0f), Random::NextF32());
            }
        }

        inline void OnGui()
        {
            ImGui::SliderFloat("Mod Amt", &m_modAmt, 0.0f, 1.0f);
            ImGui::SliderInt("Main Offset", &m_mainOffset, -36, 36);
            ImGui::SliderFloat("Main Env.", &m_mainEnv.y, 1.0f, 40.0f);
            ImGui::SliderFloat("Main Vol.", &m_mainMix.x, -5.0f, 0.0f);
            ImGui::SliderFloat("Main Pan", &m_mainMix.y, 0.0f, 1.0f);
        }

        inline void OnNote(f32 note)
        {
            m_mainFreq = NoteToFreq(note + (f32)m_mainOffset);
            m_mainEnv.x = 0.0f;
            for (i32 i = 0; i < NumOscs; ++i)
            {
                m_modFreqs[i] = NoteToFreq(-12.0f + note + i * 12.0f);
                m_modEnvs[i].x = 0.0f;
            }
        }

        inline float2 OnTick()
        {
            const f32* modFreqs = m_modFreqs;
            float2* modPhase = m_modPhase;
            float2* modEnvs = m_modEnvs;
            const float2* modMix = m_modMix;

            float2 modulation = 0.0f;
            for (i32 i = 0; i < NumOscs; ++i)
            {
                modulation += Mix(SineOsc(modPhase[i], modFreqs[i]) * ImpulseEnv(modEnvs[i]), modMix[i]);
            }

            m_mainPhase += modulation * Scale * m_modAmt;

            return Mix(SineOsc(m_mainPhase, m_mainFreq) * ImpulseEnv(m_mainEnv), m_mainMix);
        }
    };

    struct Synth
    {
        f32 m_gate = 0.0f;
        f32 m_note = 41.0f;
        Voice m_voice;

        inline void OnGui()
        {
            if (ImGui::Button("Randomize"))
            {
                m_voice.Randomize();
            }
            if (ImGui::Button("Trigger"))
            {
                m_gate = 1.0f;
            }
            ImGui::Value("Gate", m_gate);
            ImGui::Value("Note", m_note);
            m_voice.OnGui();
        }

        inline void OnNote(f32 note)
        {
            m_voice.OnNote(note);
        }

        inline float2 OnTick()
        {
            float2 value = m_voice.OnTick();
            m_gate = 0.0f;
            return math::reinhard(value);
        }
    };

    static Synth ms_synth;

    static void KeyListener(InputEvent evt)
    {
        if (evt.id >= KeyCode_A && evt.id <= KeyCode_Z)
        {
            if (evt.value > 0.0f)
            {
                ms_synth.OnNote((f32)evt.id);
            }
        }
    }

    static void AudioMain(f32* buffer, i32 num_frames, i32 num_channels)
    {
        float2* buffer2 = (float2*)buffer;
        for (i32 i = 0; i < num_frames; i++)
        {
            *buffer2++ = ms_synth.OnTick();
        }
    }

    void Init()
    {
        Random::Seed();
        ms_synth.m_voice.Randomize();

        saudio_desc desc = {};
        desc.num_channels = 2;
        desc.sample_rate = 44100;
        desc.stream_cb = AudioMain;
        saudio_setup(&desc);

        InputSystem::Listen(InputChannel_Keyboard, KeyListener);
    }

    void Update()
    {

    }

    void Shutdown()
    {
        saudio_shutdown();

        InputSystem::Deafen(InputChannel_Keyboard, KeyListener);
    }

    void Visualize()
    {
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        if (ImGui::Begin("Audio System"))
        {
            ms_synth.OnGui();
        }
        ImGui::End();
    }

    System GetSystem()
    {
        System sys;
        sys.Init = Init;
        sys.Update = Update;
        sys.Shutdown = Shutdown;
        sys.Visualize = Visualize;
        sys.enabled = true;
        sys.visualizing = true;
        return sys;
    }
};
