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

#pragma once

#define RTMIDI_VERSION "2.1.1"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace RtMidiError
{
    enum Type
    {
        WARNING,           /*!< A non-critical error. */
        DEBUG_WARNING,     /*!< A non-critical error which might be useful for debugging. */
        UNSPECIFIED,       /*!< The default, unspecified error type. */
        NO_DEVICES_FOUND,  /*!< No devices found on system. */
        INVALID_DEVICE,    /*!< An invalid device ID was specified. */
        MEMORY_ERROR,      /*!< An error occured during memory allocation. */
        INVALID_PARAMETER, /*!< An invalid parameter was specified to a function. */
        INVALID_USE,       /*!< The function was called incorrectly. */
        DRIVER_ERROR,      /*!< A system driver error occured. */
        SYSTEM_ERROR,      /*!< A system error occured. */
        THREAD_ERROR       /*!< A thread error occured. */
    };
};

typedef void(*RtMidiErrorCallback)(
    RtMidiError::Type type,
    const std::string &errorText,
    void *userData);

typedef void(*RtMidiCallback)(
    double timeStamp,
    std::vector<unsigned char> *message,
    void *userData);

class MidiApi;

class RtMidi
{
public:

    enum Api
    {
        UNSPECIFIED,    /*!< Search for a working compiled API. */
        MACOSX_CORE,    /*!< Macintosh OS-X Core Midi API. */
        LINUX_ALSA,     /*!< The Advanced Linux Sound Architecture API. */
        UNIX_JACK,      /*!< The JACK Low-Latency MIDI Server API. */
        WINDOWS_MM,     /*!< The Microsoft Multimedia MIDI API. */
        RTMIDI_DUMMY    /*!< A compilable but non-functional API. */
    };

    RtMidi() {}
    virtual ~RtMidi() {}

    virtual void openPort(unsigned int portNumber, const std::string portName) = 0;
    virtual void openVirtualPort(const std::string portName) = 0;
    virtual unsigned int getPortCount() = 0;
    virtual std::string getPortName(unsigned int portNumber) = 0;
    virtual void closePort(void) = 0;
    virtual bool isPortOpen(void) const = 0;
    virtual void setErrorCallback(RtMidiErrorCallback errorCallback, void *userData) = 0;

};

class RtMidiIn : public RtMidi
{
public:
    virtual void setCallback(RtMidiCallback callback, void *userData) = 0;
    virtual void cancelCallback() = 0;
    virtual void ignoreTypes(bool midiSysex, bool midiTime, bool midiSense) = 0;
    virtual double getMessage(std::vector<unsigned char>& message) = 0;
};

class RtMidiOut : public RtMidi
{
public:
    virtual void sendMessage(std::vector<unsigned char> *message) = 0;
};

class MidiApi
{
public:
    virtual ~MidiApi() {}
    virtual RtMidi::Api getCurrentApi(void) = 0;
    virtual void openPort(unsigned int portNumber, const std::string portName) = 0;
    virtual void openVirtualPort(const std::string portName) = 0;
    virtual void closePort(void) = 0;

    virtual unsigned int getPortCount(void) = 0;
    virtual std::string getPortName(unsigned int portNumber) = 0;

    inline bool isPortOpen() const { return m_connected; }
    void setErrorCallback(RtMidiErrorCallback errorCallback, void *userData);

    void error(RtMidiError::Type type, std::string errorString);

protected:
    virtual bool initialize(const std::string& clientName) = 0;

    void *m_apiData;
    bool m_connected;
    RtMidiErrorCallback m_errorCallback;
    bool m_firstErrorOccurred;
    void *m_errorCallbackUserData;
};

class MidiInApi : public MidiApi
{
public:

    MidiInApi(unsigned int queueSizeLimit);
    virtual ~MidiInApi();
    void setCallback(RtMidiCallback callback, void *userData);
    void cancelCallback(void);
    virtual void ignoreTypes(bool midiSysex, bool midiTime, bool midiSense);
    double getMessage(std::vector<unsigned char> *message);

    // A MIDI structure used internally by the class to store incoming
    // messages.  Each message represents one and only one MIDI message.
    struct MidiMessage
    {
        std::vector<unsigned char> bytes;
        double timeStamp;
        MidiMessage() : bytes(0), timeStamp(0.0) {}
    };

    struct MidiQueue
    {
        unsigned int front;
        unsigned int back;
        unsigned int size;
        unsigned int ringSize;
        MidiMessage *ring;

        MidiQueue() : front(0), back(0), size(0), ringSize(0), ring(NULL) {}
    };

    struct RtMidiInData
    {
        MidiQueue queue;
        MidiMessage message;
        unsigned char ignoreFlags;
        bool doInput;
        bool firstMessage;
        void *apiData;
        bool usingCallback;
        RtMidiCallback userCallback;
        void *userData;
        bool continueSysex;

        RtMidiInData() :
            ignoreFlags(7),
            doInput(false),
            firstMessage(true),
            apiData(NULL),
            usingCallback(false),
            userCallback(NULL),
            userData(NULL),
            continueSysex(false)
        {}
    };

protected:
    RtMidiInData m_inputData;
};

class MidiOutApi : public MidiApi
{
public:
    virtual void sendMessage(std::vector<unsigned char> *message) = 0;
};

class MidiInWinMM final : public MidiInApi
{
public:
    MidiInWinMM(const std::string clientName, unsigned int queueSizeLimit);
    ~MidiInWinMM(void);
    RtMidi::Api getCurrentApi(void) { return RtMidi::WINDOWS_MM; };
    void openPort(unsigned int portNumber, const std::string portName);
    void openVirtualPort(const std::string portName);
    void closePort(void);
    unsigned int getPortCount(void);
    std::string getPortName(unsigned int portNumber);

protected:
    bool initialize(const std::string& clientName);
};

class MidiOutWinMM final : public MidiOutApi
{
public:
    MidiOutWinMM(const std::string clientName);
    ~MidiOutWinMM(void);
    RtMidi::Api getCurrentApi(void) { return RtMidi::WINDOWS_MM; };
    void openPort(unsigned int portNumber, const std::string portName);
    void openVirtualPort(const std::string portName);
    void closePort(void);
    unsigned int getPortCount(void);
    std::string getPortName(unsigned int portNumber);
    void sendMessage(std::vector<unsigned char> *message);

protected:
    bool initialize(const std::string& clientName);
};
