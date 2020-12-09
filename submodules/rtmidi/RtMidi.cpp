/**********************************************************************/
/*! \class RtMidi
    \brief An abstract base class for realtime MIDI input/output.

    This class implements some common functionality for the realtime
    MIDI input/output subclasses RtMidiIn and RtMidiOut.

    RtMidi WWW site: http://music.mcgill.ca/~gary/rtmidi/

    RtMidi: realtime MIDI i/o C++ classes
    Copyright (c) 2003-2016 Gary P. Scavone

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation files
    (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software,
    and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    Any person wishing to distribute modifications to the Software is
    asked to send the modifications to the original developer so that
    they can be incorporated into the canonical version.  This is,
    however, not a binding provision of this license.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/**********************************************************************/

#if 0

#include "RtMidi.h"
#include <sstream>

void MidiApi::setErrorCallback(RtMidiErrorCallback errorCallback, void *userData)
{
    m_errorCallback = errorCallback;
    m_errorCallbackUserData = userData;
}

void MidiApi::error(RtMidiError::Type type, std::string errorString)
{
    if (m_errorCallback)
    {
        if (m_firstErrorOccurred)
            return;

        m_firstErrorOccurred = true;
        m_errorCallback(type, errorString, m_errorCallbackUserData);
        m_firstErrorOccurred = false;
    }
}

MidiInApi::MidiInApi(unsigned int queueSizeLimit)
    : MidiApi()
{
    m_inputData.queue.ringSize = queueSizeLimit;
    if (m_inputData.queue.ringSize > 0)
    {
        m_inputData.queue.ring = new MidiMessage[m_inputData.queue.ringSize];
    }
}

MidiInApi::~MidiInApi()
{
    if (m_inputData.queue.ringSize > 0)
    {
        delete[] m_inputData.queue.ring;
    }
}

void MidiInApi::setCallback(RtMidiCallback callback, void *userData)
{
    if (m_inputData.usingCallback)
    {
        error(RtMidiError::WARNING, "MidiInApi::setCallback: a callback function is already set!");
        return;
    }

    if (!callback)
    {
        error(RtMidiError::WARNING, "RtMidiIn::setCallback: callback function value is invalid!");
        return;
    }

    m_inputData.userCallback = callback;
    m_inputData.userData = userData;
    m_inputData.usingCallback = true;
}

void MidiInApi::cancelCallback()
{
    if (!m_inputData.usingCallback)
    {
        error(RtMidiError::WARNING, "RtMidiIn::cancelCallback: no callback function was set!");
        return;
    }

    m_inputData.userCallback = 0;
    m_inputData.userData = 0;
    m_inputData.usingCallback = false;
}

void MidiInApi::ignoreTypes(bool midiSysex, bool midiTime, bool midiSense)
{
    m_inputData.ignoreFlags = 0;
    if (midiSysex) m_inputData.ignoreFlags = 0x01;
    if (midiTime) m_inputData.ignoreFlags |= 0x02;
    if (midiSense) m_inputData.ignoreFlags |= 0x04;
}

double MidiInApi::getMessage(std::vector<unsigned char> *message)
{
    message->clear();

    if (m_inputData.usingCallback)
    {
        error(RtMidiError::WARNING, "RtMidiIn::getNextMessage: a user callback is currently set for this port.");
        return 0.0;
    }

    if (m_inputData.queue.size == 0) return 0.0;

    // Copy queued message to the vector pointer argument and then "pop" it.
    std::vector<unsigned char> *bytes = &(m_inputData.queue.ring[m_inputData.queue.front].bytes);
    message->assign(bytes->begin(), bytes->end());
    double deltaTime = m_inputData.queue.ring[m_inputData.queue.front].timeStamp;
    m_inputData.queue.size--;
    m_inputData.queue.front++;
    if (m_inputData.queue.front == m_inputData.queue.ringSize)
        m_inputData.queue.front = 0;

    return deltaTime;
}

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)

// API information deciphered from:
//  - http://msdn.microsoft.com/library/default.asp?url=/library/en-us/multimed/htm/_win32_midi_reference.asp

// Thanks to Jean-Baptiste Berruchon for the sysex code.

// The Windows MM API is based on the use of a callback function for
// MIDI input.  We convert the system specific time stamps to delta
// time values.

// Windows MM MIDI header files.
#include <windows.h>
#include <mmsystem.h>

#define  RT_SYSEX_BUFFER_SIZE 1024
#define  RT_SYSEX_BUFFER_COUNT 4

// A structure to hold variables related to the CoreMIDI API
// implementation.
struct WinMidiData
{
    HMIDIIN inHandle;    // Handle to Midi Input Device
    HMIDIOUT outHandle;  // Handle to Midi Output Device
    DWORD lastTime;
    MidiInApi::MidiMessage message;
    LPMIDIHDR sysexBuffer[RT_SYSEX_BUFFER_COUNT];
    CRITICAL_SECTION _mutex; // [Patrice] see https://groups.google.com/forum/#!topic/mididev/6OUjHutMpEo
};

//*********************************************************************//
//  API: Windows MM
//  Class Definitions: MidiInWinMM
//*********************************************************************//

static void CALLBACK midiInputCallback(HMIDIIN /*hmin*/,
    UINT inputStatus,
    DWORD_PTR instancePtr,
    DWORD_PTR midiMessage,
    DWORD timestamp)
{
    if (inputStatus != MIM_DATA && inputStatus != MIM_LONGDATA && inputStatus != MIM_LONGERROR) return;

    //MidiInApi::RtMidiInData *data = static_cast<MidiInApi::RtMidiInData *> (instancePtr);
    MidiInApi::RtMidiInData *data = (MidiInApi::RtMidiInData *)instancePtr;
    WinMidiData *apiData = static_cast<WinMidiData *> (data->apiData);

    // Calculate time stamp.
    if (data->firstMessage == true)
    {
        apiData->message.timeStamp = 0.0;
        data->firstMessage = false;
    }
    else apiData->message.timeStamp = (double)(timestamp - apiData->lastTime) * 0.001;
    apiData->lastTime = timestamp;

    if (inputStatus == MIM_DATA)
    { // Channel or system message

// Make sure the first byte is a status byte.
        unsigned char status = (unsigned char)(midiMessage & 0x000000FF);
        if (!(status & 0x80)) return;

        // Determine the number of bytes in the MIDI message.
        unsigned short nBytes = 1;
        if (status < 0xC0) nBytes = 3;
        else if (status < 0xE0) nBytes = 2;
        else if (status < 0xF0) nBytes = 3;
        else if (status == 0xF1)
        {
            if (data->ignoreFlags & 0x02) return;
            else nBytes = 2;
        }
        else if (status == 0xF2) nBytes = 3;
        else if (status == 0xF3) nBytes = 2;
        else if (status == 0xF8 && (data->ignoreFlags & 0x02))
        {
            // A MIDI timing tick message and we're ignoring it.
            return;
        }
        else if (status == 0xFE && (data->ignoreFlags & 0x04))
        {
            // A MIDI active sensing message and we're ignoring it.
            return;
        }

        // Copy bytes to our MIDI message.
        unsigned char *ptr = (unsigned char *)&midiMessage;
        for (int i = 0; i < nBytes; ++i) apiData->message.bytes.push_back(*ptr++);
    }
    else
    { // Sysex message ( MIM_LONGDATA or MIM_LONGERROR )
        MIDIHDR *sysex = (MIDIHDR *)midiMessage;
        if (!(data->ignoreFlags & 0x01) && inputStatus != MIM_LONGERROR)
        {
            // Sysex message and we're not ignoring it
            for (int i = 0; i < (int)sysex->dwBytesRecorded; ++i)
                apiData->message.bytes.push_back(sysex->lpData[i]);
        }

        // The WinMM API requires that the sysex buffer be requeued after
        // input of each sysex message.  Even if we are ignoring sysex
        // messages, we still need to requeue the buffer in case the user
        // decides to not ignore sysex messages in the future.  However,
        // it seems that WinMM calls this function with an empty sysex
        // buffer when an application closes and in this case, we should
        // avoid requeueing it, else the computer suddenly reboots after
        // one or two minutes.
        if (apiData->sysexBuffer[sysex->dwUser]->dwBytesRecorded > 0)
        {
            //if ( sysex->dwBytesRecorded > 0 ) {
            EnterCriticalSection(&(apiData->_mutex));
            MMRESULT result = midiInAddBuffer(apiData->inHandle, apiData->sysexBuffer[sysex->dwUser], sizeof(MIDIHDR));
            LeaveCriticalSection(&(apiData->_mutex));
            if (result != MMSYSERR_NOERROR)
            {
                puts("\nRtMidiIn::midiInputCallback: error sending sysex to Midi device!!");
            }

            if (data->ignoreFlags & 0x01) return;
        }
        else return;
    }

    if (data->usingCallback)
    {
        RtMidiCallback callback = data->userCallback;
        callback(apiData->message.timeStamp, &apiData->message.bytes, data->userData);
    }
    else
    {
        // As long as we haven't reached our queue size limit, push the message.
        if (data->queue.size < data->queue.ringSize)
        {
            data->queue.ring[data->queue.back++] = apiData->message;
            if (data->queue.back == data->queue.ringSize)
                data->queue.back = 0;
            data->queue.size++;
        }
        else
        {
            puts("\nRtMidiIn: message queue limit reached!!");
        }
    }

    // Clear the vector for the next input message.
    apiData->message.bytes.clear();
}

MidiInWinMM::MidiInWinMM(const std::string clientName, unsigned int queueSizeLimit) : MidiInApi(queueSizeLimit)
{
    initialize(clientName);
}

MidiInWinMM :: ~MidiInWinMM()
{
    // Close a connection if it exists.
    closePort();

    WinMidiData *data = static_cast<WinMidiData *> (m_apiData);
    DeleteCriticalSection(&(data->_mutex));

    // Cleanup.
    delete data;
}

bool MidiInWinMM::initialize(const std::string& /*clientName*/)
{
    // We'll issue a warning here if no devices are available but not
    // throw an error since the user can plugin something later.
    unsigned int nDevices = midiInGetNumDevs();
    if (nDevices == 0)
    {
        error(RtMidiError::WARNING, "MidiInWinMM::initialize: no MIDI input devices currently available.");
        return false;
    }

    // Save our api-specific connection information.
    WinMidiData *data = (WinMidiData *) new WinMidiData;
    m_apiData = (void *)data;
    m_inputData.apiData = (void *)data;
    data->message.bytes.clear();  // needs to be empty for first input message

    if (!InitializeCriticalSectionAndSpinCount(&(data->_mutex), 0x00000400))
    {
        error(RtMidiError::WARNING, "MidiInWinMM::initialize: InitializeCriticalSectionAndSpinCount failed.");
        return false;
    }
    return true;
}

void MidiInWinMM::openPort(unsigned int portNumber, const std::string /*portName*/)
{
    if (m_connected)
    {
        error(RtMidiError::WARNING, "MidiInWinMM::openPort: a valid connection already exists!");
        return;
    }

    unsigned int nDevices = midiInGetNumDevs();
    if (nDevices == 0)
    {
        error(RtMidiError::NO_DEVICES_FOUND, "MidiInWinMM::openPort: no MIDI input sources found!");
        return;
    }

    if (portNumber >= nDevices)
    {
        std::ostringstream ost;
        ost << "MidiInWinMM::openPort: the 'portNumber' argument (" << portNumber << ") is invalid.";
        error(RtMidiError::INVALID_PARAMETER, ost.str());
        return;
    }

    WinMidiData *data = static_cast<WinMidiData *> (m_apiData);
    MMRESULT result = midiInOpen(&data->inHandle,
        portNumber,
        (DWORD_PTR)&midiInputCallback,
        (DWORD_PTR)&m_inputData,
        CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR)
    {
        error(RtMidiError::DRIVER_ERROR, "MidiInWinMM::openPort: error creating Windows MM MIDI input port.");
        return;
    }

    // Allocate and init the sysex buffers.
    for (int i = 0; i < RT_SYSEX_BUFFER_COUNT; ++i)
    {
        data->sysexBuffer[i] = (MIDIHDR*) new char[sizeof(MIDIHDR)];
        data->sysexBuffer[i]->lpData = new char[RT_SYSEX_BUFFER_SIZE];
        data->sysexBuffer[i]->dwBufferLength = RT_SYSEX_BUFFER_SIZE;
        data->sysexBuffer[i]->dwUser = i; // We use the dwUser parameter as buffer indicator
        data->sysexBuffer[i]->dwFlags = 0;

        result = midiInPrepareHeader(data->inHandle, data->sysexBuffer[i], sizeof(MIDIHDR));
        if (result != MMSYSERR_NOERROR)
        {
            midiInClose(data->inHandle);
            error(RtMidiError::DRIVER_ERROR, "MidiInWinMM::openPort: error starting Windows MM MIDI input port (PrepareHeader).");
            return;
        }

        // Register the buffer.
        result = midiInAddBuffer(data->inHandle, data->sysexBuffer[i], sizeof(MIDIHDR));
        if (result != MMSYSERR_NOERROR)
        {
            midiInClose(data->inHandle);
            error(RtMidiError::DRIVER_ERROR, "MidiInWinMM::openPort: error starting Windows MM MIDI input port (AddBuffer).");
            return;
        }
    }

    result = midiInStart(data->inHandle);
    if (result != MMSYSERR_NOERROR)
    {
        midiInClose(data->inHandle);
        error(RtMidiError::DRIVER_ERROR, "MidiInWinMM::openPort: error starting Windows MM MIDI input port.");
        return;
    }

    m_connected = true;
}

void MidiInWinMM::openVirtualPort(std::string /*portName*/)
{
    // This function cannot be implemented for the Windows MM MIDI API.
    error(RtMidiError::WARNING, "MidiInWinMM::openVirtualPort: cannot be implemented in Windows MM MIDI API!");
}

void MidiInWinMM::closePort(void)
{
    if (m_connected)
    {
        WinMidiData *data = static_cast<WinMidiData *> (m_apiData);
        EnterCriticalSection(&(data->_mutex));
        midiInReset(data->inHandle);
        midiInStop(data->inHandle);

        for (int i = 0; i < RT_SYSEX_BUFFER_COUNT; ++i)
        {
            int result = midiInUnprepareHeader(data->inHandle, data->sysexBuffer[i], sizeof(MIDIHDR));
            delete[] data->sysexBuffer[i]->lpData;
            delete[] data->sysexBuffer[i];
            if (result != MMSYSERR_NOERROR)
            {
                midiInClose(data->inHandle);
                error(RtMidiError::DRIVER_ERROR, "MidiInWinMM::openPort: error closing Windows MM MIDI input port (midiInUnprepareHeader).");
                return;
            }
        }

        midiInClose(data->inHandle);
        m_connected = false;
        LeaveCriticalSection(&(data->_mutex));
    }
}

unsigned int MidiInWinMM::getPortCount()
{
    return midiInGetNumDevs();
}

std::string MidiInWinMM::getPortName(unsigned int portNumber)
{
    std::string stringName;
    unsigned int nDevices = midiInGetNumDevs();
    if (portNumber >= nDevices)
    {
        std::ostringstream ost;
        ost << "MidiInWinMM::getPortName: the 'portNumber' argument (" << portNumber << ") is invalid.";
        error(RtMidiError::WARNING, ost.str());
        return stringName;
    }

    MIDIINCAPS deviceCaps;
    midiInGetDevCaps(portNumber, &deviceCaps, sizeof(MIDIINCAPS));

    stringName = std::string(deviceCaps.szPname);

    // Next lines added to add the portNumber to the name so that 
    // the device's names are sure to be listed with individual names
    // even when they have the same brand name
    std::ostringstream os;
    os << " ";
    os << portNumber;
    stringName += os.str();

    return stringName;
}

MidiOutWinMM::MidiOutWinMM(const std::string clientName) : MidiOutApi()
{
    initialize(clientName);
}

MidiOutWinMM::~MidiOutWinMM()
{
    closePort();
    if (m_apiData)
    {
        WinMidiData* data = static_cast<WinMidiData*>(m_apiData);
        delete data;
        m_apiData = NULL;
    }
}

bool MidiOutWinMM::initialize(const std::string& /*clientName*/)
{
    // We'll issue a warning here if no devices are available but not
    // throw an error since the user can plug something in later.
    unsigned int nDevices = midiOutGetNumDevs();
    if (nDevices == 0)
    {
        error(RtMidiError::WARNING, "MidiOutWinMM::initialize: no MIDI output devices currently available.");
        return false;
    }

    WinMidiData* data = new WinMidiData;
    m_apiData = (void*)data;
    return true;
}

unsigned int MidiOutWinMM::getPortCount()
{
    return midiOutGetNumDevs();
}

std::string MidiOutWinMM::getPortName(unsigned int portNumber)
{
    std::string stringName;
    unsigned int nDevices = midiOutGetNumDevs();
    if (portNumber >= nDevices)
    {
        std::ostringstream ost;
        ost << "MidiOutWinMM::getPortName: the 'portNumber' argument (" << portNumber << ") is invalid.";
        error(RtMidiError::WARNING, ost.str());
        return stringName;
    }

    MIDIOUTCAPS deviceCaps;
    midiOutGetDevCaps(portNumber, &deviceCaps, sizeof(MIDIOUTCAPS));

    stringName = std::string(deviceCaps.szPname);

    // Next lines added to add the portNumber to the name so that 
    // the device's names are sure to be listed with individual names
    // even when they have the same brand name
    std::ostringstream os;
    os << " ";
    os << portNumber;
    stringName += os.str();

    return stringName;
}

void MidiOutWinMM::openPort(unsigned int portNumber, const std::string /*portName*/)
{
    if (m_connected)
    {
        error(RtMidiError::WARNING, "MidiOutWinMM::openPort: a valid connection already exists!");
        return;
    }

    unsigned int nDevices = midiOutGetNumDevs();
    if (nDevices < 1)
    {
        error(RtMidiError::NO_DEVICES_FOUND, "MidiOutWinMM::openPort: no MIDI output destinations found!");
        return;
    }

    if (portNumber >= nDevices)
    {
        std::ostringstream ost;
        ost << "MidiOutWinMM::openPort: the 'portNumber' argument (" << portNumber << ") is invalid.";
        error(RtMidiError::INVALID_PARAMETER, ost.str());
        return;
    }

    WinMidiData *data = static_cast<WinMidiData *> (m_apiData);
    MMRESULT result = midiOutOpen(&data->outHandle,
        portNumber,
        (DWORD)NULL,
        (DWORD)NULL,
        CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR)
    {
        error(RtMidiError::DRIVER_ERROR, "MidiOutWinMM::openPort: error creating Windows MM MIDI output port.");
        return;
    }

    m_connected = true;
}

void MidiOutWinMM::closePort(void)
{
    if (m_connected)
    {
        WinMidiData *data = static_cast<WinMidiData *> (m_apiData);
        midiOutReset(data->outHandle);
        midiOutClose(data->outHandle);
        m_connected = false;
    }
}

void MidiOutWinMM::openVirtualPort(std::string /*portName*/)
{
    error(RtMidiError::WARNING, "MidiOutWinMM::openVirtualPort: cannot be implemented in Windows MM MIDI API!");
}

void MidiOutWinMM::sendMessage(std::vector<unsigned char> *message)
{
    if (!m_connected) return;

    unsigned int nBytes = static_cast<unsigned int>(message->size());
    if (nBytes == 0)
    {
        error(RtMidiError::WARNING, "MidiOutWinMM::sendMessage: message argument is empty!");
        return;
    }

    MMRESULT result;
    WinMidiData *data = static_cast<WinMidiData *> (m_apiData);
    if (message->at(0) == 0xF0)
    { // Sysex message

// Allocate buffer for sysex data.
        char *buffer = (char *)malloc(nBytes);
        if (buffer == NULL)
        {
            error(RtMidiError::MEMORY_ERROR, "MidiOutWinMM::sendMessage: error allocating sysex message memory!");
            return;
        }

        // Copy data to buffer.
        for (unsigned int i = 0; i < nBytes; ++i) buffer[i] = message->at(i);

        // Create and prepare MIDIHDR structure.
        MIDIHDR sysex;
        sysex.lpData = (LPSTR)buffer;
        sysex.dwBufferLength = nBytes;
        sysex.dwFlags = 0;
        result = midiOutPrepareHeader(data->outHandle, &sysex, sizeof(MIDIHDR));
        if (result != MMSYSERR_NOERROR)
        {
            free(buffer);
            error(RtMidiError::DRIVER_ERROR, "MidiOutWinMM::sendMessage: error preparing sysex header.");
            return;
        }

        // Send the message.
        result = midiOutLongMsg(data->outHandle, &sysex, sizeof(MIDIHDR));
        if (result != MMSYSERR_NOERROR)
        {
            free(buffer);
            error(RtMidiError::DRIVER_ERROR, "MidiOutWinMM::sendMessage: error sending sysex message.");
            return;
        }

        // Unprepare the buffer and MIDIHDR.
        while (MIDIERR_STILLPLAYING == midiOutUnprepareHeader(data->outHandle, &sysex, sizeof(MIDIHDR))) Sleep(1);
        free(buffer);
    }
    else
    { // Channel or system message.

// Make sure the message size isn't too big.
        if (nBytes > 3)
        {
            error(RtMidiError::WARNING, "MidiOutWinMM::sendMessage: message size is greater than 3 bytes (and not sysex)!");
            return;
        }

        // Pack MIDI bytes into double word.
        DWORD packet;
        unsigned char *ptr = (unsigned char *)&packet;
        for (unsigned int i = 0; i < nBytes; ++i)
        {
            *ptr = message->at(i);
            ++ptr;
        }

        // Send the message immediately.
        result = midiOutShortMsg(data->outHandle, packet);
        if (result != MMSYSERR_NOERROR)
        {
            error(RtMidiError::DRIVER_ERROR, "MidiOutWinMM::sendMessage: error sending MIDI message.");
        }
    }
}

#endif // Windows

#endif // 0
