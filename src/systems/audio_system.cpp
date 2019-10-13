#include "audio_system.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "math/math.h"
#include "common/find.h"

#include <sokol/sokol_audio.h>

namespace AudioSystem
{
    constexpr f32 SampleFreq = 44100.0f;
    constexpr f32 DeltaTime = 1.0f / SampleFreq;

    enum ProcessorId
    {
        ProcessorId_None = 0,
        ProcessorId_Mov,
        ProcessorId_Inc,
        ProcessorId_Dec,
        ProcessorId_Zero,
        ProcessorId_One,
        ProcessorId_Add,
        ProcessorId_Sub,
        ProcessorId_Mul,
        ProcessorId_Div,
        ProcessorId_Mod,
        ProcessorId_Lerp,
        ProcessorId_Saturate,
        ProcessorId_Fract,
        ProcessorId_Sin,
        ProcessorId_Cos,
        ProcessorId_Tan,
        ProcessorId_Select,
        ProcessorId_Step,
        ProcessorId_SmoothStep,
        ProcessorId_Exp2,
        ProcessorId_Log2,
        ProcessorId_NoteToFreq,
        ProcessorId_FreqToNote,
        ProcessorId_SineWave,
        ProcessorId_Impulse,
        ProcessorId_ZeroCrossing,
        ProcessorId_Gate,
        ProcessorId_PhaseMod,
        ProcessorId_Reinhard,
    };

    enum SignalId
    {
        SignalId_None = 0,
        SignalId_Note,      // [0, 128] midi note
        SignalId_Velocity,  // [0, 1] midi velocity
        SignalId_Pitch,     // midi frequency, in hz
        SignalId_Gate,      // midi gate, 1 on velocity zero crossing, otherwise 0
        SignalId_Out,
    };

    struct Processor
    {
        ProcessorId id;
        FixedArray<i32, 8> inputs;
        FixedArray<i32, 4> outputs;
    };

    inline f32 NoteToFreq(f32 note)
    {
        constexpr f32 i = 1.0f / 12.0f;
        return math::exp2(i * (note - 69.0f)) * 440.0f;
    }

    inline f32 FreqToNote(f32 freq)
    {
        constexpr f32 j = 1.0f / 440.0f;
        return (math::log2(freq * j) * 12.0f) + 69.0f;
    }

    inline f32 Gate(f32 trigger, f32 phase)
    {
        if (trigger > 0.0f)
        {
            return 0.0f;
        }
        return phase;
    }

    inline f32 PhaseMod(f32 phase, f32 signal)
    {
        return math::frac(phase + signal * DeltaTime);
    }

    inline f32 SineWave(f32& phase, f32 freq)
    {
        phase = PhaseMod(phase, freq);
        return math::sin(phase * math::Tau);
    }

    inline f32 Impulse(f32 gate, f32& phase, f32 k)
    {
        if (gate > 0.0f)
        {
            phase = 0.0f;
        }
        f32 h = k * phase;
        phase += DeltaTime;
        return math::saturate(h * math::exp(1.0f - h));
    }

    inline f32 ZeroCrossing(f32 a, f32 b)
    {
        return (a > 0.0f && b <= 0.0f) ? 1.0f : 0.0f;
    }

    static void Tick(const Processor& proc, Slice<f32> memory)
    {
        const auto& in = proc.inputs;
        const auto& out = proc.outputs;
        switch (proc.id)
        {
        default:
        case ProcessorId_None:
            break;
        case ProcessorId_Zero:
            memory[out[0]] = 0.0f;
            break;
        case ProcessorId_One:
            memory[out[0]] = 1.0f;
            break;
        case ProcessorId_Inc:
            ++memory[out[0]];
            break;
        case ProcessorId_Dec:
            --memory[out[0]];
            break;
        case ProcessorId_Mov:
            memory[out[0]] = memory[in[0]];
            break;
        case ProcessorId_Add:
            memory[out[0]] = memory[in[0]] + memory[in[1]];
            break;
        case ProcessorId_Sub:
            memory[out[0]] = memory[in[0]] - memory[in[1]];
            break;
        case ProcessorId_Mul:
            memory[out[0]] = memory[in[0]] * memory[in[1]];
            break;
        case ProcessorId_Div:
            memory[out[0]] = memory[in[0]] / memory[in[1]];
            break;
        case ProcessorId_Mod:
            memory[out[0]] = math::fmod(memory[in[0]], memory[in[1]]);
            break;
        case ProcessorId_Lerp:
            memory[out[0]] = math::lerp(memory[in[0]], memory[in[1]], memory[in[2]]);
            break;
        case ProcessorId_Saturate:
            memory[out[0]] = math::saturate(memory[in[0]]);
            break;
        case ProcessorId_Fract:
            memory[out[0]] = math::frac(memory[in[0]]);
            break;
        case ProcessorId_Sin:
            memory[out[0]] = math::sin(memory[in[0]]);
            break;
        case ProcessorId_Cos:
            memory[out[0]] = math::cos(memory[in[0]]);
            break;
        case ProcessorId_Tan:
            memory[out[0]] = math::tan(memory[in[0]]);
            break;
        case ProcessorId_Select:
            memory[out[0]] = math::select(memory[in[0]], memory[in[1]], memory[in[2]] != 0.0f);
            break;
        case ProcessorId_Step:
            memory[out[0]] = math::step(memory[in[0]], memory[in[1]]);
            break;
        case ProcessorId_SmoothStep:
            memory[out[0]] = math::smoothstep(memory[in[0]], memory[in[1]], memory[in[2]]);
            break;
        case ProcessorId_Exp2:
            memory[out[0]] = math::exp2(memory[in[0]]);
            break;
        case ProcessorId_Log2:
            memory[out[0]] = math::log2(memory[in[0]]);
            break;
        case ProcessorId_NoteToFreq:
            memory[out[0]] = NoteToFreq(memory[in[0]]);
            break;
        case ProcessorId_FreqToNote:
            memory[out[0]] = FreqToNote(memory[in[0]]);
            break;
        case ProcessorId_SineWave:
            memory[out[0]] = SineWave(memory[in[0]], memory[in[1]]);
            break;
        case ProcessorId_Impulse:
            memory[out[0]] = Impulse(memory[in[0]], memory[in[1]], memory[in[2]]);
            break;
        case ProcessorId_ZeroCrossing:
            memory[out[0]] = ZeroCrossing(memory[in[0]], memory[in[1]]);
            break;
        case ProcessorId_Gate:
            memory[out[0]] = Gate(memory[in[0]], memory[in[1]]);
            break;
        case ProcessorId_PhaseMod:
            memory[out[0]] = PhaseMod(memory[in[0]], memory[in[1]]);
            break;
        case ProcessorId_Reinhard:
            memory[out[0]] = math::reinhard(memory[in[0]]);
            break;
        }
    }

    static void Tick(
        Slice<Processor> processors,
        Slice<f32> memory)
    {
        for (const Processor& proc : processors)
        {
            Tick(proc, memory);
        }
    }

    struct ProcessorMemory
    {
        Array<f32> memory;
        Array<i32> refCount;
        Array<i32> freelist;

        inline Slice<f32> Get()
        {
            return { memory.begin(), memory.size() };
        }
        inline i32 Allocate()
        {
            if (freelist.empty())
            {
                freelist.grow() = memory.size();
                memory.grow() = 0.0f;
                refCount.grow() = 0;
            }
            i32 i = freelist.popValue();
            memory[i] = 0.0f;
            refCount[i] = 1;
            return i;
        }
        inline void Free(i32 i)
        {
            i32 rc = --refCount[i];
            if (rc == 0)
            {
                freelist.grow() = i;
                memory[i] = 0.0f;
            }
        }
        inline void Reset()
        {
            memory.reset();
            refCount.reset();
            freelist.reset();
        }
    };

    struct Phasor
    {
        f32 m_phase;
        f32 m_freqMult;
        f32 m_freqBias;

        inline f32 OnTick(f32 freq)
        {
            freq = m_freqBias + m_freqMult * freq;
            return SineWave(m_phase, freq);
        }
        inline f32 OnTickPM(f32 freq, f32 phaseMod)
        {
            m_phase += phaseMod;
            return OnTick(freq);
        }
        inline f32 OnTickReset(f32 freq, float2 reset)
        {
            if (ZeroCrossing(reset.x, reset.y))
            {
                m_phase = 0.0f;
            }
            return OnTick(freq);
        }
    };

    struct Envelope
    {
        f32 m_phase;
        f32 m_duration;

        inline f32 OnTick(f32 gate, f32 velocity)
        {
            return Impulse(gate, m_phase, m_duration) * velocity;
        }
    };

    struct MixChannel
    {
        f32 m_volume;
        f32 m_pan;

        inline float2 Mix(f32 x) const
        {
            f32 pan = math::to_unorm(m_pan);
            return float2(1.0f - pan, pan) * m_volume * x;
        }
        inline float2 Mix(float2 x) const
        {
            f32 pan = math::to_unorm(m_pan);
            return float2(1.0f - pan, pan) * m_volume * x;
        }
    };

    struct Subvoice
    {
        Phasor m_phasor;
        Envelope m_envelope;
        MixChannel m_mixer;

        inline float2 OnTick(f32 gate, f32 freq, f32 velocity)
        {
            return m_mixer.Mix(
                m_phasor.OnTick(freq) *
                m_envelope.OnTick(gate, velocity));
        }
    };

    struct Voice
    {
        Subvoice m_voices[16];

        float2 OnTick(f32 gate, f32 freq, f32 velocity)
        {
            float2 value = 0.0f;
            for (Subvoice& voice : m_voices)
            {
                value += voice.OnTick(gate, freq, velocity);
            }
            return value;
        }
    };

    struct Synth
    {
        u8 m_notes[64];
        f32 m_freqs[64];
        f32 m_velocities[64];
        f32 m_gates[64];
        f32 m_durations[64];
        Voice m_voices[64];

        float2 OnTick(Slice<u8> notes, Slice<u8> velocities)
        {
            for (i32 i = 0; i < notes.size(); ++i)
            {
                f32 freq = NoteToFreq((f32)notes[i]);
                f32 velocity = (f32)velocities[i] / 255.0f;

                i32 v = Find(argof(m_notes), notes[i]);
                if (v == -1)
                {
                    v = FindMax(argof(m_durations));
                    m_velocities[v] = 0.0f;
                }

                m_gates[v] = ZeroCrossing(velocity, m_velocities[v]);
                if (m_gates[v] > 0.0f)
                {
                    m_durations[v] = 0.0f;
                }

                m_notes[v] = notes[i];
                m_freqs[v] = freq;
                m_velocities[v] = velocity;
            }

            float2 value = 0.0f;
            for (i32 i = 0; i < countof(m_voices); ++i)
            {
                value += m_voices[i].OnTick(
                    m_gates[i],
                    m_freqs[i],
                    m_velocities[i]);
                m_durations[i] += DeltaTime;
            }

            return math::reinhard(value);
        }
    };

    static void AudioMain(f32* buffer, i32 num_frames, i32 num_channels)
    {
        for (i32 i = 0; i < num_frames; i++)
        {
            buffer[2 * i + 0] = 0.0f;     // left channel
            buffer[2 * i + 1] = 0.0f;     // right channel
        }
    }

    void Init()
    {
        saudio_desc desc = {};
        desc.num_channels = 2;
        desc.sample_rate = 44100;
        desc.stream_cb = AudioMain;
        saudio_setup(&desc);
    }

    void Update()
    {

    }

    void Shutdown()
    {
        saudio_shutdown();
    }

    void Visualize()
    {

    }

    System GetSystem()
    {
        System sys;
        sys.Init = Init;
        sys.Update = Update;
        sys.Shutdown = Shutdown;
        sys.Visualize = Visualize;
        sys.enabled = true;
        sys.visualizing = false;
        return sys;
    }
};
