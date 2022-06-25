#include "audio/audio_system.h"

#include "common/atomics.h"
#include "audio/midi_system.h"
#include "common/profiler.h"
#include "allocator/allocator.h"
#include "math/types.h"
#include "math/float2_funcs.h"

#include <portaudio.h>
#include <ui/cimgui_ext.h>
#include <string.h>

#define REQUESTED_SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 256
#define NUM_CHANNELS 2

// ----------------------------------------------------------------------------

typedef enum
{
    AudioEventType_NULL = 0,
    AudioEventType_MIDI,

    AudioEventType_COUNT
} AudioEventType;

static char const* const AudioEventType_Strings[] =
{
    "NULL",
    "MIDI",
};
SASSERT(NELEM(AudioEventType_Strings) == AudioEventType_COUNT);

typedef struct AudioEvent_s
{
    AudioEventType type;
    u32 tick;
    union
    {
        MidiMsg midi;
    };
} AudioEvent;

typedef struct AudioEventSegment_s
{
    u32 iWrite;
    u32 iRead;
    AudioEvent events[64];
} AudioEventSegment;

typedef struct AudioEventSegmentRing_s
{
    u32 index;
    AudioEventSegment segments[2];
} AudioEventSegmentRing;

typedef struct MidiCon_s
{
    MidiHdl handle;
    i32 port;
} MidiCon;

// ----------------------------------------------------------------------------

static void* AudioAlloc(size_t size, void* userData);
static void AudioFree(void* ptr, void* userData);
static AudioEventSegment* GetAudioEvents_Reader(void);
static AudioEventSegment* GetAudioEvents_Writer(void);
static u32 AudioEventSegment_GetCount(const AudioEventSegment* seg);
static bool AudioEventSegment_Push(AudioEventSegment* seg, const AudioEvent* evtIn);
static bool AudioEventSegment_Pop(AudioEventSegment* seg, AudioEvent* evtOut);
static void OnMidiEventFn(const MidiMsg* msg, void* usr);
static int OnAudioPacketFn(
    const void* inputBuffer,
    void* outputBuffer, u64 frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData);
static float2 OnAudioTickFn(u32 tick);
static void OnAudioEventFn(const AudioEvent* evt);

// ----------------------------------------------------------------------------

static AudioEventSegmentRing ms_eventRing;
static u32 ms_tick;
static i32 ms_bufferSize;
static i32 ms_sampleRate;
static float ms_secondsPerTick;
static i32 ms_midiCount;
static MidiCon* ms_midis;
static PaStream* ms_stream;

// ----------------------------------------------------------------------------

void AudioSys_Init(void)
{
    i32 err = Pa_Initialize();
    ASSERT(err == 0);

    i32 device = Pa_GetDefaultOutputDevice();
    ASSERT(device >= 0);

    const PaStreamParameters outputParams = {
        .channelCount = NUM_CHANNELS,
        .sampleFormat = paFloat32,
        .device = device,
        .suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency,
    };

    err = Pa_OpenDefaultStream(
        &ms_stream,
        0,
        NUM_CHANNELS,
        paFloat32,
        REQUESTED_SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        OnAudioPacketFn,
        NULL);
    ASSERT(err == 0);

    PaStreamInfo* info = Pa_GetStreamInfo(ms_stream);
    ms_sampleRate = info->sampleRate;
    ms_bufferSize = FRAMES_PER_BUFFER;
    ms_secondsPerTick = 1.0f / ms_sampleRate;

    err = Pa_StartStream(ms_stream);
    ASSERT(err == 0);
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
    const i32 midiCount = ms_midiCount; ms_midiCount = 0;
    MidiCon* midis = ms_midis; ms_midis = NULL;
    for (i32 i = 0; i < midiCount; ++i)
    {
        Midi_Close(midis[i].handle);
    }
    Mem_Free(midis);

    MidiSys_Shutdown();
    Pa_CloseStream(ms_stream);
    Pa_Terminate();
}

ProfileMark(pm_ongui, AudioSys_Gui)
void AudioSys_Gui(bool* pEnabled)
{
    ProfileBegin(pm_ongui);

    if (igBegin("Audio", pEnabled, 0))
    {

        if (igTreeNodeStr("Midi Devices"))
        {
            const i32 portCount = Midi_DeviceCount();
            if (portCount > ms_midiCount)
            {
                Perm_Reserve(ms_midis, portCount);
                for (i32 i = ms_midiCount; i < portCount; ++i)
                {
                    ms_midis[i] = (MidiCon) { .port = i };
                }
                ms_midiCount = portCount;
            }
            igText("Midi device count: %d", portCount);

            const i32 conCount = ms_midiCount;
            for (i32 port = 0; port < conCount; ++port)
            {
                MidiCon* con = &ms_midis[port];
                ASSERT(con->port == port);
                bool connected = Midi_Exists(con->handle);
                igText("Port %d state: %s", port, connected ? "Connected" : "Disconnected");
                if (connected)
                {
                    igExSameLine();
                    if (igExButton("Disconnect"))
                    {
                        Midi_Close(con->handle);
                    }
                }
                else
                {
                    igExSameLine();
                    if (igExButton("Connect"))
                    {
                        con->handle = Midi_Open(port, OnMidiEventFn, NULL);
                    }
                }
                ASSERT(con->port == port);
            }

            igTreePop(); // Midi Devices
        }

        if (igTreeNodeStr("Audio Events"))
        {
            igText("Current Tick: %u", load_u32(&ms_tick, MO_Acquire));

            const AudioEventSegmentRing* ring = &ms_eventRing;
            u32 ringOffset = load_u32(&ring->index, MO_Acquire);
            for (u32 i = 0; i < NELEM(ring->segments); ++i)
            {
                u32 iSeg = (i + ringOffset) % NELEM(ring->segments);
                const AudioEventSegment* seg = &ring->segments[iSeg];
                u32 segOffset = load_u32(&seg->iRead, MO_Acquire);
                for (u32 j = 0; j < NELEM(seg->events); ++j)
                {
                    u32 iEvt = (j + segOffset) % NELEM(seg->events);
                    const AudioEvent* evt = &seg->events[iEvt];
                    ASSERT((u32)evt->type < AudioEventType_COUNT);
                    if (evt->type != AudioEventType_NULL)
                    {
                        igText("Tick: %u", evt->tick);
                        igText("Type: %s", AudioEventType_Strings[evt->type]);
                        switch (evt->type)
                        {
                        default:
                            ASSERT(false);
                            break;
                        case AudioEventType_MIDI:
                            igText("Port: %d", evt->midi.port);
                            igText("Command: %d", evt->midi.command);
                            igText("Param 1: %d", evt->midi.param1);
                            igText("Param 2: %d", evt->midi.param2);
                            break;
                        }
                        igSeparator();
                    }
                }
            }

            igTreePop(); // Audio Events
        }

    }
    igEnd(); // Audio

    ProfileEnd(pm_ongui);
}

// ----------------------------------------------------------------------------

static void* AudioAlloc(size_t size, void* userData)
{
    ASSERT((i32)size >= 0);
    return Perm_Alloc((i32)size);
}

static void AudioFree(void* ptr, void* userData)
{
    Mem_Free(ptr);
}

static AudioEventSegment* GetAudioEvents_Reader(void)
{
    u32 segIdx = fetch_add_u32(&ms_eventRing.index, 1, MO_AcqRel);
    return &ms_eventRing.segments[segIdx % NELEM(ms_eventRing.segments)];
}

static AudioEventSegment* GetAudioEvents_Writer(void)
{
    u32 segIdx = load_u32(&ms_eventRing.index, MO_Acquire) + 1;
    return &ms_eventRing.segments[segIdx % NELEM(ms_eventRing.segments)];
}

static u32 AudioEventSegment_GetCount(const AudioEventSegment* seg)
{
    return load_u32(&seg->iWrite, MO_Acquire) - load_u32(&seg->iRead, MO_Acquire);
}

static bool AudioEventSegment_Push(AudioEventSegment* seg, const AudioEvent* evtIn)
{
    const u32 kMask = NELEM(seg->events) - 1u;
    if (AudioEventSegment_GetCount(seg) < kMask)
    {
        u32 i = fetch_add_u32(&seg->iWrite, 1, MO_AcqRel);
        i &= kMask;
        memcpy(&seg->events[i], evtIn, sizeof(*evtIn));
        return true;
    }
    return false;
}

static bool AudioEventSegment_Pop(AudioEventSegment* seg, AudioEvent* evtOut)
{
    memset(evtOut, 0, sizeof(*evtOut));
    const u32 kMask = NELEM(seg->events) - 1u;
    if (AudioEventSegment_GetCount(seg) != 0)
    {
        u32 i = fetch_add_u32(&seg->iRead, 1, MO_AcqRel);
        i &= kMask;
        memcpy(evtOut, &seg->events[i], sizeof(*evtOut));
        memset(&seg->events[i], 0, sizeof(seg->events[i]));
        return true;
    }
    return false;
}

static void OnMidiEventFn(const MidiMsg* msg, void* usr)
{
    u32 tick = load_u32(&ms_tick, MO_Acquire) + (u32)ms_bufferSize;
    AudioEventSegment* seg = GetAudioEvents_Writer();
    const AudioEvent evt =
    {
        .type = AudioEventType_MIDI,
        .tick = tick,
        .midi = *msg,
    };
    AudioEventSegment_Push(seg, &evt);
}

static int OnAudioPacketFn(
    const void* input,
    void* output,
    u64 frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    float scratch[16] = { 0 };
    AudioEventSegment* seg = GetAudioEvents_Reader();
    float* outputBuffer = output;

    if (NUM_CHANNELS > 0 && NUM_CHANNELS <= NELEM(scratch))
    {
        AudioEvent evt = { 0 };
        AudioEventSegment_Pop(seg, &evt);
        for (i32 i = 0; i < frameCount; ++i)
        {
            u32 tick = fetch_add_u32(&ms_tick, 1, MO_AcqRel);
            while (evt.tick == tick)
            {
                OnAudioEventFn(&evt);
                if (!AudioEventSegment_Pop(seg, &evt))
                {
                    break;
                }
            }
            float2 value = OnAudioTickFn(tick);
            scratch[0] = value.x;
            scratch[1] = value.y;
            memcpy(outputBuffer, scratch, sizeof(outputBuffer[0]) * NUM_CHANNELS);
            outputBuffer += NUM_CHANNELS;
        }
    }

    return paContinue;
}

static float2 OnAudioTickFn(u32 tick)
{
    // TODO: implement
    return f2_0;
}

static void OnAudioEventFn(const AudioEvent* evt)
{
    switch (evt->type)
    {
    default:
        ASSERT(false);
        break;
    case AudioEventType_NULL:
        break;
    case AudioEventType_MIDI:
        // TODO: implement
        break;
    }
}
