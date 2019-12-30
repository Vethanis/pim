#include "audio_system.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "math/math.h"
#include "common/find.h"
#include "input/input_system.h"
#include "ui/imgui.h"
#include "common/random.h"
#include "components/system.h"

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

    static void AudioMain(f32* buffer, i32 num_frames, i32 num_channels)
    {
        float2* buffer2 = (float2*)buffer;
        for (i32 i = 0; i < num_frames; i++)
        {
            *buffer2++ = 0.0f;
        }
    }

    static void KeyListener(InputEvent evt, f32 valueX, f32 valueY)
    {

    }

    static void Init()
    {
        saudio_desc desc = {};
        desc.num_channels = 2;
        desc.sample_rate = 44100;
        desc.stream_cb = AudioMain;
        saudio_setup(&desc);

        InputSystem::Register(InputChannel_Keyboard, KeyListener);
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        saudio_shutdown();

        InputSystem::Remove(InputChannel_Keyboard, KeyListener);
    }

    static constexpr Guid ms_dependencies[] =
    {
        ToGuid("InputSystem"),
    };

    static constexpr System ms_system =
    {
        ToGuid("AudioSystem"),
        { ARGS(ms_dependencies) },
        Init,
        Update,
        Shutdown,
    };
    static RegisterSystem ms_register(ms_system);
};
