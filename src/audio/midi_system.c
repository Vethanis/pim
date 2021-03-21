#include "audio/midi_system.h"
#include "audio/audio_system.h"
#include "threading/spinlock.h"
#include "common/console.h"
#include "allocator/allocator.h"
#include "containers/queue.h"
#include <string.h>

// https://docs.microsoft.com/en-us/windows/win32/multimedia/managing-midi-recording
#include <windows.h>
#include <mmsystem.h>

// ----------------------------------------------------------------------------

typedef struct sysex_s
{
    MIDIHDR header;
    u8 buffer[1024];
} sysex_t;

typedef struct midicon_s
{
    spinlock_t lock;
    HMIDIIN handle;
    midi_cb cb;
    void* usr;
    sysex_t buffers[4];
} midicon_t;

// ----------------------------------------------------------------------------

static void midicon_lock(midicon_t* con);
static void midicon_unlock(midicon_t* con);

static bool midicon_open(midicon_t* con, i32 port, midi_cb cb, void* usr);
static void midicon_close(midicon_t* con);
static bool midicon_enqueue(midicon_t* con, i32 bufferId);
static void midicon_push(midicon_t* con, const midimsg_t* src);
static void midicon_result(MMRESULT result);

static void __stdcall InputCallback(
    HMIDIIN handle,
    UINT status,
    DWORD_PTR user,
    DWORD_PTR message,
    DWORD timestamp);

// ----------------------------------------------------------------------------

static void midicon_lock(midicon_t* con)
{
    spinlock_lock(&con->lock);
}

static void midicon_unlock(midicon_t* con)
{
    spinlock_unlock(&con->lock);
}

static bool midicon_open(midicon_t* con, i32 port, midi_cb cb, void* usr)
{
    MMRESULT result = MMSYSERR_NOERROR;

    memset(con, 0, sizeof(*con));
    spinlock_new(&con->lock);
    con->cb = cb;
    con->usr = usr;

    result = midiInOpen(
        &con->handle,
        port,
        (DWORD_PTR)InputCallback,
        (DWORD_PTR)con,
        CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR)
    {
        goto onfail;
    }
    for (i32 i = 0; i < NELEM(con->buffers); ++i)
    {
        sysex_t* buffer = &con->buffers[i];
        buffer->header.lpData = (LPSTR)&buffer->buffer[0];
        buffer->header.dwBufferLength = sizeof(buffer->buffer);
        buffer->header.dwUser = i;
        result = midiInPrepareHeader(con->handle, &buffer->header, sizeof(buffer->header));
        if (result != MMSYSERR_NOERROR)
        {
            goto onfail;
        }
        result = midiInAddBuffer(con->handle, &buffer->header, sizeof(buffer->header));
        if (result != MMSYSERR_NOERROR)
        {
            goto onfail;
        }
    }
    result = midiInStart(con->handle);
    if (result != MMSYSERR_NOERROR)
    {
        goto onfail;
    }

    return true;

onfail:
    midicon_result(result);
    midicon_close(con);
    return false;
}

static void midicon_close(midicon_t* con)
{
    if (con)
    {
        midicon_lock(con);
        if (con->handle)
        {
            midiInStop(con->handle);
            midiInReset(con->handle);
            for (i32 i = 0; i < NELEM(con->buffers); ++i)
            {
                sysex_t* buffer = &con->buffers[i];
                midiInUnprepareHeader(con->handle, &buffer->header, sizeof(buffer->header));
            }
            midiInClose(con->handle);
            con->handle = NULL;
        }
        midicon_unlock(con);

        spinlock_del(&con->lock);
        memset(con, 0, sizeof(*con));
    }
}

static bool midicon_enqueue(midicon_t* con, i32 bufferId)
{
    MMRESULT result = MMSYSERR_INVALHANDLE;
    midicon_lock(con);
    if (con->handle)
    {
        result = MMSYSERR_NOERROR;
        if (con->buffers[bufferId].header.dwBytesRecorded > 0)
        {
            result = midiInAddBuffer(
                con->handle,
                &con->buffers[bufferId].header,
                sizeof(con->buffers[bufferId].header));
        }
    }
    midicon_unlock(con);
    midicon_result(result);
    return result == MMSYSERR_NOERROR;
}

static void midicon_push(midicon_t* con, const midimsg_t* src)
{
    midi_cb cb = con->cb;
    void* usr = con->usr;
    cb(src, usr);
}

static void midicon_result(MMRESULT result)
{
    if (result != MMSYSERR_NOERROR)
    {
        char buffer[PIM_PATH] = { 0 };
        midiInGetErrorTextA(result, buffer, sizeof(buffer));
        buffer[NELEM(buffer) - 1] = 0;
        Con_Logf(LogSev_Error, "midi", "%s", buffer);
    }
}

static void __stdcall InputCallback(
    HMIDIIN handle,
    UINT status,
    DWORD_PTR user,
    DWORD_PTR message,
    DWORD timestamp)
{
    midicon_t* con = (midicon_t*)user;
    switch (status)
    {
    default:
        break;
    case MIM_DATA:
    {
        midimsg_t msg = { 0 };
        msg.tick = AudioSys_Ticks();
        msg.command = (message >> 0) & 0xff;
        msg.param1 = (message >> 8) & 0xff;
        msg.param2 = (message >> 16) & 0xff;
        if (msg.command >= 0x80)
        {
            midicon_push(con, &msg);
        }
    }
    break;
    case MIM_LONGDATA:
    case MIM_LONGERROR:
    {
        ASSERT(message);
        MIDIHDR* sysex = (MIDIHDR*)message;
        i32 bufferId = (i32)sysex->dwUser;
        ASSERT(bufferId >= 0);
        ASSERT(bufferId < NELEM(con->buffers));
        midicon_enqueue(con, bufferId);
    }
    break;
    }
}

// ----------------------------------------------------------------------------

static queue_t ms_free;
static i32 ms_length;
static u8* ms_versions;
static midicon_t* ms_cons;

void midi_sys_init(void)
{
    queue_create(&ms_free, sizeof(i32), EAlloc_Perm);
}

void midi_sys_update(void)
{

}

void midi_sys_shutdown(void)
{
    midicon_t* cons = ms_cons;
    const i32 len = ms_length;
    for (i32 i = 0; i < len; ++i)
    {
        midicon_close(&cons[i]);
    }

    ms_length = 0;
    Mem_Free(ms_cons); ms_cons = NULL;
    Mem_Free(ms_versions); ms_versions = NULL;
    queue_destroy(&ms_free);
}

i32 midi_devcount(void)
{
    return midiInGetNumDevs();
}

bool midi_exists(midihdl_t hdl)
{
    i32 index = hdl.index;
    u8 version = hdl.version;
    return (index < ms_length) && (ms_versions[index] == version);
}

midihdl_t midi_open(i32 port, midi_cb cb, void* usr)
{
    ASSERT(port >= 0);
    ASSERT(cb);

    midihdl_t hdl = { 0 };
    if ((port < 0) || (port >= midi_devcount()))
    {
        return hdl;
    }
    if (!cb)
    {
        return hdl;
    }

    i32 index = 0;
    if (!queue_trypop(&ms_free, &index, sizeof(index)))
    {
        index = ms_length++;
        Perm_Grow(ms_versions, ms_length);
        Perm_Grow(ms_cons, ms_length);
    }
    midicon_t* con = &ms_cons[index];
    if (midicon_open(con, port, cb, usr))
    {
        u8 version = ++ms_versions[index];
        ASSERT(ms_versions[index] & 1);
        hdl.version = version;
        hdl.index = index;
        return hdl;
    }
    else
    {
        queue_push(&ms_free, &index, sizeof(index));
        return hdl;
    }
}

bool midi_close(midihdl_t hdl)
{
    if (midi_exists(hdl))
    {
        i32 index = hdl.index;
        ++ms_versions[index];
        ASSERT(!(ms_versions[index] & 1));
        midicon_close(&ms_cons[index]);
        return true;
    }
    return false;
}
