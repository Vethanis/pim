#pragma once

#ifdef _MSC_VER

/**********************************************************************
 *
 * StackWalker.h
 *
 *
 *
 * LICENSE (http://www.opensource.org/licenses/bsd-license.php)
 *
 *   Copyright (c) 2005-2009, Jochen Kalmbach
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without modification,
 *   are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *   Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *   Neither the name of Jochen Kalmbach nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 *   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * **********************************************************************/

#include <stdint.h>

namespace StackWalker
{
    constexpr uint32_t MaxNameLen = 256;

    enum CallstackEntryType : int32_t
    {
        firstEntry,
        nextEntry,
        lastEntry
    };

    struct Walker;

    struct CallstackEntry
    {
        const char* symTypeString;
        uint64_t offset;
        uint64_t offsetFromSmybol;
        uint64_t baseOfImage;
        uint32_t symType;
        uint32_t offsetFromLine;
        uint32_t lineNumber;
        char name[MaxNameLen];
        char undName[MaxNameLen];
        char undFullName[MaxNameLen];
        char lineFileName[MaxNameLen];
        char moduleName[MaxNameLen];
        char loadedImageName[MaxNameLen];
    };

    using OnCallstackEntryFn = void(*)(
        Walker& walker,
        CallstackEntryType eType,
        CallstackEntry& entry);

    using OnOutputFn = void(*)(
        Walker& walker,
        const char* text);

    using OnErrorFn = void(*)(
        Walker& walker,
        const char* funcName,
        uint32_t errorCode,
        uint64_t address);

    struct Walker
    {
        OnCallstackEntryFn OnCallstackEntry;
        OnOutputFn OnOutput;
        OnErrorFn OnError;
    };

    bool Init(Walker& walker);
    bool ShowCallstack(Walker& walker);
    bool ShowObject(Walker& walker, void* pObject);

    // ------------------------------------------------------------------------

    void DefaultOnCallstackEntry(
        Walker& walker,
        CallstackEntryType eType,
        CallstackEntry& entry);

    void DefaultOnOutput(
        Walker& walker,
        const char* text);

    void DefaultOnError(
        Walker& walker,
        const char* funcName,
        uint32_t errorCode,
        uint64_t address);

    static void EmptyOnCallstackEntry(
        Walker& walker,
        CallstackEntryType eType,
        CallstackEntry& entry) {}

    static void EmptyOnOutput(
        Walker& walker,
        const char* text) {}

    static void EmptyOnError(
        Walker& walker,
        const char* funcName,
        uint32_t errorCode,
        uint64_t address) {}
};

#endif // _MSC_VER
