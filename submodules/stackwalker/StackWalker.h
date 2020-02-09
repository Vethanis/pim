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
    enum CallstackEntryType : int32_t
    {
        firstEntry,
        nextEntry,
        lastEntry
    };

    struct CallstackEntry
    {
        const char* symTypeString;
        uint64_t offset;
        uint64_t offsetFromSmybol;
        uint64_t baseOfImage;
        uint32_t symType;
        uint32_t offsetFromLine;
        uint32_t lineNumber;
        char name[260];
        char undName[260];
        char undFullName[260];
        char lineFileName[260];
        char moduleName[260];
        char loadedImageName[260];
    };

    struct Walker
    {
        bool Init();
        bool ShowCallstack();
        bool ShowObject(void* pObject);

        virtual void OnCallstackEntry(CallstackEntryType eType, CallstackEntry& entry);
        virtual void OnOutput(const char* text);
        virtual void OnError(const char* funcName, uint32_t errCode, uint64_t address);
        virtual void OnSymInit(const char* searchPath, uint32_t symOptions, const char* userName);
        virtual void OnLoadModule(
            const char* img,
            const char* mod,
            uint64_t baseAddr,
            uint32_t size,
            uint32_t result,
            const char* symType,
            const char* pdbName,
            uint64_t fileVersion);
    };
};

#endif // _MSC_VER
