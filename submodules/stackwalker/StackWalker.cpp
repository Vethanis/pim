/**********************************************************************
 *
 * StackWalker.cpp
 * https://github.com/JochenKalmbach/StackWalker
 *
 * Old location: http://stackwalker.codeplex.com/
 *
 *
 * History:
 *  2005-07-27   v1    - First public release on http://www.codeproject.com/
 *                       http://www.codeproject.com/threads/StackWalker.asp
 *  2005-07-28   v2    - Changed the params of the constructor and ShowCallstack
 *                       (to simplify the usage)
 *  2005-08-01   v3    - Changed to use 'CONTEXT_FULL' instead of CONTEXT_ALL
 *                       (should also be enough)
 *                     - Changed to compile correctly with the PSDK of VC7.0
 *                       (GetFileVersionInfoSizeA and GetFileVersionInfoA is wrongly defined:
 *                        it uses LPSTR instead of LPCSTR as first parameter)
 *                     - Added declarations to support VC5/6 without using 'dbghelp.h'
 *                     - Added a 'pUserData' member to the ShowCallstack function and the
 *                       PReadProcessMemoryRoutine declaration (to pass some user-defined data,
 *                       which can be used in the readMemoryFunction-callback)
 *  2005-08-02   v4    - OnSymInit now also outputs the OS-Version by default
 *                     - Added example for doing an exception-callstack-walking in main.cpp
 *                       (thanks to owillebo: http://www.codeproject.com/script/profile/whos_who.asp?id=536268)
 *  2005-08-05   v5    - Removed most Lint (http://www.gimpel.com/) errors... thanks to Okko Willeboordse!
 *  2008-08-04   v6    - Fixed Bug: Missing LEAK-end-tag
 *                       http://www.codeproject.com/KB/applications/leakfinder.aspx?msg=2502890#xx2502890xx
 *                       Fixed Bug: Compiled with "WIN32_LEAN_AND_MEAN"
 *                       http://www.codeproject.com/KB/applications/leakfinder.aspx?msg=1824718#xx1824718xx
 *                       Fixed Bug: Compiling with "/Wall"
 *                       http://www.codeproject.com/KB/threads/StackWalker.aspx?msg=2638243#xx2638243xx
 *                       Fixed Bug: Now checking SymUseSymSrv
 *                       http://www.codeproject.com/KB/threads/StackWalker.aspx?msg=1388979#xx1388979xx
 *                       Fixed Bug: Support for recursive function calls
 *                       http://www.codeproject.com/KB/threads/StackWalker.aspx?msg=1434538#xx1434538xx
 *                       Fixed Bug: Missing FreeLibrary call in "GetModuleListTH32"
 *                       http://www.codeproject.com/KB/threads/StackWalker.aspx?msg=1326923#xx1326923xx
 *                       Fixed Bug: SymDia is number 7, not 9!
 *  2008-09-11   v7      For some (undocumented) reason, dbhelp.h is needing a packing of 8!
 *                       Thanks to Teajay which reported the bug...
 *                       http://www.codeproject.com/KB/applications/leakfinder.aspx?msg=2718933#xx2718933xx
 *  2008-11-27   v8      Debugging Tools for Windows are now stored in a different directory
 *                       Thanks to Luiz Salamon which reported this "bug"...
 *                       http://www.codeproject.com/KB/threads/StackWalker.aspx?msg=2822736#xx2822736xx
 *  2009-04-10   v9      License slightly corrected (<ORGANIZATION> replaced)
 *  2009-11-01   v10     Moved to http://stackwalker.codeplex.com/
 *  2009-11-02   v11     Now try to use IMAGEHLP_MODULE64_V3 if available
 *  2010-04-15   v12     Added support for VS2010 RTM
 *  2010-05-25   v13     Now using secure MyStrcCpy. Thanks to luke.simon:
 *                       http://www.codeproject.com/KB/applications/leakfinder.aspx?msg=3477467#xx3477467xx
 *  2013-01-07   v14     Runtime Check Error VS2010 Debug Builds fixed:
 *                       http://stackwalker.codeplex.com/workitem/10511
 *
 *
 * LICENSE (http://www.opensource.org/licenses/bsd-license.php)
 *
 *   Copyright (c) 2005-2013, Jochen Kalmbach
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
 **********************************************************************/

#include "StackWalker.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <dbghelp.h>
#include <tlhelp32.h>
#include <Psapi.h>

#pragma comment(lib, "version.lib") // "VerQueryValue"
#pragma warning(disable : 4826)
#pragma warning(disable : 4996)

#define USED_CONTEXT_FLAGS CONTEXT_FULL

#define NELEM(x) ( sizeof(x) / sizeof((x)[0]) )

 // ----------------------------------------------------------------------------

struct BaseProc
{
    const char* name;
    FARPROC proc;

    bool Load(HMODULE hModule)
    {
        proc = GetProcAddress(hModule, name);
        return proc != 0;
    }
};

template<typename T>
struct DynProc
{
    BaseProc base;

    BaseProc& AsBase() { return base; }
    operator T*() { return reinterpret_cast<T*>(base.proc); }
};

#define DeclProc(T) DynProc<decltype(T)> T##Fn = { #T };

// ----------------------------------------------------------------------------

static bool FileExists(const char* path)
{
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

template<uint32_t nSize>
static bool GetEnvVar(const char* name, char(&buffer)[nSize])
{
    uint32_t rv = GetEnvironmentVariableA(name, buffer, nSize);
    return (rv != 0) && (rv < nSize);
}

static bool GetOsVersion(OSVERSIONINFOA& version)
{
    memset(&version, 0, sizeof(version));
    version.dwOSVersionInfoSize = sizeof(version);
    return GetVersionExA(&version);
}

static uint64_t GetFileVersion(const char* filename)
{
    uint64_t fileVersion = 0;
    VS_FIXEDFILEINFO* fInfo = NULL;
    DWORD dwHandle;
    uint32_t dwSize = GetFileVersionInfoSizeA(filename, &dwHandle);
    if (dwSize)
    {
        void* vData = malloc(dwSize);
        if (vData)
        {
            if (GetFileVersionInfoA(filename, dwHandle, dwSize, vData))
            {
                uint32_t len;
                if (VerQueryValueA(vData, "\\", (void**)&fInfo, &len))
                {
                    fileVersion =
                        ((uint64_t)fInfo->dwFileVersionLS) +
                        ((uint64_t)fInfo->dwFileVersionMS << 32);
                }
            }
            free(vData);
        }
    }
    return fileVersion;
}

static BOOL ReadProcMem(
    HANDLE hProcess,
    DWORD64 qwBaseAddress,
    PVOID lpBuffer,
    DWORD nSize,
    LPDWORD lpNumberOfBytesRead)
{
    SIZE_T st = 0;
    BOOL rv = ReadProcessMemory(
        hProcess,
        (void*)qwBaseAddress,
        lpBuffer,
        nSize,
        &st);
    *lpNumberOfBytesRead = (DWORD)st;
    return rv;
}

// ----------------------------------------------------------------------------

namespace StackWalker
{
    static char ms_symbolPath[MAX_PATH];
    static HANDLE ms_hProcess;
    static HANDLE ms_hThread;
    static DWORD ms_processId;

    void DefaultOnOutput(Walker& walker, const char* buffer)
    {
        OutputDebugStringA(buffer);
    }

    static void OnOutput(Walker& walker, const char* text)
    {
        walker.OnOutput(walker, text);
    }

    static void OnCallstackEntry(
        Walker& walker,
        CallstackEntryType eType,
        CallstackEntry& entry)
    {
        walker.OnCallstackEntry(walker, eType, entry);
    }

    void DefaultOnCallstackEntry(
        Walker& walker,
        CallstackEntryType eType,
        CallstackEntry& entry)
    {
        char buffer[MaxNameLen];

        if ((eType != lastEntry) && (entry.offset))
        {
            if (!entry.name[0])
            {
                strcpy_s(entry.name, "(null)");
            }

            if (entry.undName[0])
            {
                strcpy_s(entry.name, entry.undName);
            }

            if (entry.undFullName[0])
            {
                strcpy_s(entry.name, entry.undFullName);
            }

            if (!entry.lineFileName[0])
            {
                strcpy_s(entry.lineFileName, "(null)");
                if (!entry.moduleName[0])
                {
                    strcpy_s(entry.moduleName, "(null)");
                }
                sprintf_s(
                    buffer,
                    "%p (%s): %s: %s\n",
                    (void*)entry.offset,
                    entry.moduleName,
                    entry.lineFileName,
                    entry.name);
            }
            else
            {
                sprintf_s(
                    buffer,
                    "%s (%d): %s\n",
                    entry.lineFileName,
                    entry.lineNumber,
                    entry.name);
            }
            OnOutput(walker, buffer);
        }
    }

    static void OnSymInit(
        Walker& walker,
        const char* szSearchPath,
        uint32_t symOptions,
        const char* szUserName)
    {
        char buffer[MaxNameLen];
        sprintf_s(
            buffer,
            "SymInit: Symbol-SearchPath: '%s', symOptions: %d, UserName: '%s'\n",
            szSearchPath,
            symOptions,
            szUserName);
        OnOutput(walker, buffer);

        OSVERSIONINFOA ver;
        if (GetOsVersion(ver))
        {
            sprintf_s(
                buffer,
                "OS-Version: %d.%d.%d (%s)\n",
                ver.dwMajorVersion,
                ver.dwMinorVersion,
                ver.dwBuildNumber,
                ver.szCSDVersion);
            OnOutput(walker, buffer);
        }
    }

    static void OnLoadModule(
        Walker& walker,
        const char* img,
        const char* mod,
        uint64_t baseAddr,
        uint32_t size,
        uint32_t result,
        const char* symType,
        const char* pdbName,
        uint64_t fileVersion)
    {
        char buffer[MaxNameLen];

        sprintf_s(
            buffer,
            "%s:%s (%p), size: %u (result: %u), SymType: '%s', PDB: '%s'",
            img,
            mod,
            (void*)baseAddr,
            size,
            result,
            symType,
            pdbName);

        if (fileVersion)
        {
            char versionStr[64];
            uint32_t v4 = (uint32_t)(fileVersion & 0xFFFF);
            uint32_t v3 = (uint32_t)((fileVersion >> 16) & 0xFFFF);
            uint32_t v2 = (uint32_t)((fileVersion >> 32) & 0xFFFF);
            uint32_t v1 = (uint32_t)((fileVersion >> 48) & 0xFFFF);
            sprintf_s(versionStr, ", fileVersion: %u.%u.%u.%u", v1, v2, v3, v4);
            strcat_s(buffer, versionStr);
        }

        strcat_s(buffer, "\n");
        OnOutput(walker, buffer);
    }

    void DefaultOnError(
        Walker& walker,
        const char* szFuncName,
        uint32_t gle,
        uint64_t addr)
    {
        char buffer[MaxNameLen];
        sprintf_s(
            buffer,
            "ERROR: %s, GetLastError: %d (Address: %p)\n",
            szFuncName,
            gle,
            (void*)addr);
        OnOutput(walker, buffer);
    }

    static void OnError(
        Walker& walker,
        const char* szFuncName,
        uint32_t gle,
        uint64_t addr)
    {
        walker.OnError(walker, szFuncName, gle, addr);
    }

    // ----------------------------------------------------------------------------

    struct Symbol : IMAGEHLP_SYMBOL64
    {
        char NameBuffer[MaxNameLen];

        Symbol()
        {
            memset(this, 0, sizeof(*this));
            SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
            MaxNameLength = sizeof(NameBuffer);
        }

        operator IMAGEHLP_SYMBOL64* ()
        {
            return reinterpret_cast<IMAGEHLP_SYMBOL64*>(this);
        }
    };

    struct DebugHelp
    {
        HMODULE m_handle;

        BaseProc procsBegin;
        DeclProc(SymInitialize);
        DeclProc(SymCleanup);
        DeclProc(SymFunctionTableAccess64);
        DeclProc(SymGetLineFromAddr64);
        DeclProc(SymGetModuleBase64);
        DeclProc(SymGetModuleInfo64);
        DeclProc(SymGetOptions);
        DeclProc(SymGetSymFromAddr64);
        DeclProc(SymLoadModule64);
        DeclProc(SymSetOptions);
        DeclProc(StackWalk64);
        DeclProc(UnDecorateSymbolName);
        DeclProc(SymGetSearchPath);
        BaseProc procsEnd;

        BaseProc* begin() { return &procsBegin + 1; }
        BaseProc* end() { return &procsEnd; }

        HMODULE Load()
        {
            HMODULE handle = m_handle;
            if (handle)
            {
                return handle;
            }

            handle = LoadLibraryA("dbghelp.dll");

            if (!handle)
            {
                return NULL;
            }

            for (BaseProc& proc : *this)
            {
                if (!proc.Load(handle))
                {
                    FreeLibrary(handle);
                    return NULL;
                }
            }

            return handle;
        }

        bool Init(Walker& walker, const char* searchPath)
        {
            HMODULE handle = Load();
            if (!handle)
            {
                return false;
            }

            void* hProcess = ms_hProcess;

            if (!SymInitializeFn(hProcess, searchPath, false))
            {
                OnError(walker, "SymInitialize", GetLastError(), 0);
                return false;
            }

            DWORD symOptions = SymGetOptionsFn();
            symOptions |= SYMOPT_LOAD_LINES;
            symOptions |= SYMOPT_FAIL_CRITICAL_ERRORS;
            symOptions = SymSetOptionsFn(symOptions);

            char pathBuffer[MaxNameLen] = { 0 };
            if (!SymGetSearchPathFn(hProcess, pathBuffer, NELEM(pathBuffer)))
            {
                OnError(walker, "SymGetSearchPath", GetLastError(), 0);
            }

            char szUserName[MaxNameLen] = { 0 };
            DWORD dwSize = MaxNameLen;
            if (!GetUserNameA(szUserName, &dwSize))
            {
                OnError(walker, "GetUserName", GetLastError(), 0);
            }
            OnSymInit(walker, pathBuffer, symOptions, szUserName);

            m_handle = handle;

            return true;
        }

        bool Walk(STACKFRAME64& sframe, CONTEXT& ctx)
        {
            if (!m_handle)
            {
                return false;
            }
            return StackWalk64Fn(
                IMAGE_FILE_MACHINE_AMD64,
                ms_hProcess,
                ms_hThread,
                &sframe,
                &ctx,
                ReadProcMem,
                SymFunctionTableAccess64Fn,
                SymGetModuleBase64Fn,
                NULL);
        }

        uint32_t LoadModule(
            Walker& walker,
            const char* img,
            const char* mod,
            uint64_t baseAddr,
            uint32_t size)
        {
            if (!m_handle)
            {
                return ERROR_DLL_INIT_FAILED;
            }

            if (!SymLoadModule64Fn(ms_hProcess, 0, img, mod, baseAddr, size))
            {
                return GetLastError();
            }

            const uint64_t fileVersion = GetFileVersion(img);

            IMAGEHLP_MODULE64 Module;
            memset(&Module, 0, sizeof(Module));
            const char* symType = "-unknown-";
            if (SymGetModuleInfo64Fn(ms_hProcess, baseAddr, &Module))
            {
                switch (Module.SymType)
                {
                case SymNone:
                    symType = "-nosymbols-";
                    break;
                case SymCoff:
                    symType = "COFF";
                    break;
                case SymCv:
                    symType = "CV";
                    break;
                case SymPdb:
                    symType = "PDB";
                    break;
                case SymExport:
                    symType = "-exported-";
                    break;
                case SymDeferred:
                    symType = "-deferred-";
                    break;
                case SymSym:
                    symType = "SYM";
                    break;
                case SymDia:
                    symType = "DIA";
                    break;
                case SymVirtual:
                    symType = "Virtual";
                    break;
                }
            }

            const char* pdbName = Module.LoadedImageName;
            if (Module.LoadedPdbName[0])
            {
                pdbName = Module.LoadedPdbName;
            }

            OnLoadModule(
                walker,
                img, mod,
                baseAddr, size,
                ERROR_SUCCESS, symType,
                pdbName, fileVersion);

            return ERROR_SUCCESS;
        }
    };

    static DebugHelp ms_debugHelp;

    // ----------------------------------------------------------------------------

    struct ToolHelp32
    {
        HMODULE m_handle;

        BaseProc m_procsBegin;
        DeclProc(CreateToolhelp32Snapshot);
        DeclProc(Module32First);
        DeclProc(Module32Next);
        BaseProc m_procsEnd;

        BaseProc* begin() { return &m_procsBegin + 1; }
        BaseProc* end() { return &m_procsEnd; }

        bool LoadProcs(HMODULE hMod)
        {
            for (BaseProc& proc : *this)
            {
                if (!proc.Load(hMod))
                {
                    return false;
                }
            }
            return true;
        }

        bool Load()
        {
            if (m_handle)
            {
                return true;
            }
            HMODULE hMod = LoadLibrary("kernel32.dll");
            if (hMod)
            {
                if (LoadProcs(hMod))
                {
                    m_handle = hMod;
                    return true;
                }
                FreeLibrary(hMod);
            }
            return false;
        }

        bool GetModuleList(Walker& walker)
        {
            if (!Load())
            {
                return false;
            }

            HANDLE hProcess = ms_hProcess;
            const uint32_t pid = ms_processId;
            HANDLE hSnap = CreateToolhelp32SnapshotFn(TH32CS_SNAPMODULE, pid);
            if (hSnap == (HANDLE)-1)
            {
                return false;
            }

            MODULEENTRY32 modEntry;
            memset(&modEntry, 0, sizeof(modEntry));
            modEntry.dwSize = sizeof(modEntry);

            int32_t count = 0;
            bool loop = Module32FirstFn(hSnap, &modEntry);
            while (loop)
            {
                uint32_t hResult = ms_debugHelp.LoadModule(
                    walker,
                    modEntry.szExePath,
                    modEntry.szModule,
                    (uint64_t)modEntry.modBaseAddr,
                    modEntry.modBaseSize);
                if (hResult != ERROR_SUCCESS)
                {
                    OnError(walker, "LoadModule", hResult, 0);
                    OnOutput(walker, modEntry.szExePath);
                }
                else
                {
                    ++count;
                }
                loop = Module32NextFn(hSnap, &modEntry);
            }

            CloseHandle(hSnap);

            return count > 0;
        }
    };

    static ToolHelp32 ms_toolHelp32;

    // ------------------------------------------------------------------------

    static const char* GetSymbolPath()
    {
        if (ms_symbolPath[0])
        {
            return ms_symbolPath;
        }

        constexpr size_t nSymPathLen = NELEM(ms_symbolPath);
        constexpr size_t nTempLen = MAX_PATH;
        ms_symbolPath[0] = 0;
        char szTemp[nTempLen];

        if (GetCurrentDirectoryA(nTempLen, szTemp) > 0)
        {
            szTemp[nTempLen - 1] = 0;
            strcat_s(ms_symbolPath, nSymPathLen, szTemp);
            strcat_s(ms_symbolPath, nSymPathLen, ";");
        }

        if (GetModuleFileNameA(NULL, szTemp, nTempLen) > 0)
        {
            szTemp[nTempLen - 1] = 0;
            for (char* p = (szTemp + strlen(szTemp) - 1); p >= szTemp; --p)
            {
                if ((*p == '\\') || (*p == '/') || (*p == ':'))
                {
                    *p = 0;
                    break;
                }
            }
            if (strlen(szTemp) > 0)
            {
                strcat_s(ms_symbolPath, nSymPathLen, szTemp);
                strcat_s(ms_symbolPath, nSymPathLen, ";");
            }
        }

        if (GetEnvVar("_NT_SYMBOL_PATH", szTemp))
        {
            szTemp[nTempLen - 1] = 0;
            strcat_s(ms_symbolPath, nSymPathLen, szTemp);
            strcat_s(ms_symbolPath, nSymPathLen, ";");
        }

        if (GetEnvVar("_NT_ALTERNATE_SYMBOL_PATH", szTemp))
        {
            szTemp[nTempLen - 1] = 0;
            strcat_s(ms_symbolPath, nSymPathLen, szTemp);
            strcat_s(ms_symbolPath, nSymPathLen, ";");
        }

        if (GetEnvVar("SYSTEMROOT", szTemp))
        {
            szTemp[nTempLen - 1] = 0;
            strcat_s(ms_symbolPath, nSymPathLen, szTemp);
            strcat_s(ms_symbolPath, nSymPathLen, ";");
            // also add the "system32"-directory:
            strcat_s(szTemp, nTempLen, "\\system32");
            strcat_s(ms_symbolPath, nSymPathLen, szTemp);
            strcat_s(ms_symbolPath, nSymPathLen, ";");
        }

        strcat_s(ms_symbolPath, nSymPathLen,
            "SRV*c:\\websymbols*http://msdl.microsoft.com/download/symbols;");

        return ms_symbolPath[0] ? ms_symbolPath : 0;
    }

    bool Init(Walker& walker)
    {
        if (!ms_hProcess)
        {
            ms_hProcess = GetCurrentProcess();
            ms_hThread = GetCurrentThread();
            ms_processId = GetCurrentProcessId();
        }

        if (!walker.OnCallstackEntry)
        {
            walker.OnCallstackEntry = DefaultOnCallstackEntry;
        }
        if (!walker.OnError)
        {
            walker.OnError = DefaultOnError;
        }
        if (!walker.OnOutput)
        {
            walker.OnOutput = DefaultOnOutput;
        }

        bool success = true;

        if (!ms_debugHelp.Init(walker, GetSymbolPath()))
        {
            OnError(walker, "Error while initializing dbghelp.dll", 0, 0);
            SetLastError(ERROR_DLL_INIT_FAILED);
            success = false;
        }

        if (success)
        {
            success = ms_toolHelp32.GetModuleList(walker);
        }

        return success;
    }

    bool ShowCallstack(Walker& walker)
    {
        if (!walker.OnCallstackEntry)
        {
            Init(walker);
        }
        void* hProcess = ms_hProcess;

        CONTEXT ctx;
        memset(&ctx, 0, sizeof(CONTEXT));
        ctx.ContextFlags = USED_CONTEXT_FLAGS;
        RtlCaptureContext(&ctx);

        // init STACKFRAME for first call
        STACKFRAME64 sframe; // in/out stackframe
        memset(&sframe, 0, sizeof(sframe));
        DWORD imageType = IMAGE_FILE_MACHINE_AMD64;
        sframe.AddrPC.Offset = ctx.Rip;
        sframe.AddrPC.Mode = AddrModeFlat;
        sframe.AddrFrame.Offset = ctx.Rsp;
        sframe.AddrFrame.Mode = AddrModeFlat;
        sframe.AddrStack.Offset = ctx.Rsp;
        sframe.AddrStack.Mode = AddrModeFlat;

        IMAGEHLP_LINE64 Line;
        memset(&Line, 0, sizeof(Line));
        Line.SizeOfStruct = sizeof(Line);

        IMAGEHLP_MODULE64 Module;
        memset(&Module, 0, sizeof(Module));
        Module.SizeOfStruct = sizeof(Module);

        Symbol symbol;

        bool bLastEntryCalled = true;
        int32_t curRecursionCount = 0;
        CallstackEntry csEntry;
        for (int32_t frameNum = 0; ; ++frameNum)
        {
            if (!ms_debugHelp.Walk(sframe, ctx))
            {
                OnError(walker, "StackWalk64", 0, sframe.AddrPC.Offset);
                break;
            }

            csEntry.offset = sframe.AddrPC.Offset;
            csEntry.name[0] = 0;
            csEntry.undName[0] = 0;
            csEntry.undFullName[0] = 0;
            csEntry.offsetFromSmybol = 0;
            csEntry.offsetFromLine = 0;
            csEntry.lineFileName[0] = 0;
            csEntry.lineNumber = 0;
            csEntry.loadedImageName[0] = 0;
            csEntry.moduleName[0] = 0;

            if (sframe.AddrPC.Offset == sframe.AddrReturn.Offset)
            {
                if (curRecursionCount > 1000)
                {
                    OnError(walker, "StackWalk64-Endless-Callstack!", 0, sframe.AddrPC.Offset);
                    break;
                }
                curRecursionCount++;
            }
            else
            {
                curRecursionCount = 0;
            }

            if (sframe.AddrPC.Offset)
            {
                if (ms_debugHelp.SymGetSymFromAddr64Fn(
                    hProcess,
                    sframe.AddrPC.Offset,
                    (PDWORD64)&(csEntry.offsetFromSmybol),
                    symbol))
                {
                    strcpy_s(csEntry.name, symbol.Name);
                    ms_debugHelp.UnDecorateSymbolNameFn(
                        symbol.Name,
                        csEntry.undName,
                        MaxNameLen,
                        UNDNAME_NAME_ONLY);
                    ms_debugHelp.UnDecorateSymbolNameFn(
                        symbol.Name,
                        csEntry.undFullName,
                        MaxNameLen,
                        UNDNAME_COMPLETE);
                }
                else
                {
                    OnError(walker, "SymGetSymFromAddr64", GetLastError(), sframe.AddrPC.Offset);
                }

                if (ms_debugHelp.SymGetLineFromAddr64Fn(
                    hProcess,
                    sframe.AddrPC.Offset,
                    (PDWORD)&(csEntry.offsetFromLine),
                    &Line))
                {
                    csEntry.lineNumber = Line.LineNumber;
                    strcpy_s(csEntry.lineFileName, Line.FileName);
                }
                else
                {
                    OnError(walker, "SymGetLineFromAddr64", GetLastError(), sframe.AddrPC.Offset);
                }

                if (ms_debugHelp.SymGetModuleInfo64Fn(
                    hProcess,
                    sframe.AddrPC.Offset,
                    &Module))
                {
                    switch (Module.SymType)
                    {
                    case SymNone:
                        csEntry.symTypeString = "-nosymbols-";
                        break;
                    case SymCoff:
                        csEntry.symTypeString = "COFF";
                        break;
                    case SymCv:
                        csEntry.symTypeString = "CV";
                        break;
                    case SymPdb:
                        csEntry.symTypeString = "PDB";
                        break;
                    case SymExport:
                        csEntry.symTypeString = "-exported-";
                        break;
                    case SymDeferred:
                        csEntry.symTypeString = "-deferred-";
                        break;
                    case SymSym:
                        csEntry.symTypeString = "SYM";
                        break;
                    case SymDia:
                        csEntry.symTypeString = "DIA";
                        break;
                    case SymVirtual:
                        csEntry.symTypeString = "Virtual";
                        break;
                    default:
                        csEntry.symTypeString = NULL;
                        break;
                    }

                    strcpy_s(csEntry.moduleName, Module.ModuleName);
                    csEntry.baseOfImage = Module.BaseOfImage;
                    strcpy_s(csEntry.loadedImageName, Module.LoadedImageName);
                }
                else
                {
                    OnError(walker, "SymGetModuleInfo64", GetLastError(), sframe.AddrPC.Offset);
                }
            }

            bLastEntryCalled = false;
            OnCallstackEntry(walker, frameNum ? nextEntry : firstEntry, csEntry);

            if (!sframe.AddrReturn.Offset)
            {
                bLastEntryCalled = true;
                OnCallstackEntry(walker, lastEntry, csEntry);
                SetLastError(ERROR_SUCCESS);
                break;
            }
        }

        if (!bLastEntryCalled)
        {
            OnCallstackEntry(walker, lastEntry, csEntry);
        }

        return true;
    }

    bool ShowObject(Walker& walker, void* pObject)
    {
        if (!walker.OnCallstackEntry)
        {
            Init(walker);
        }
        void* hProcess = ms_hProcess;

        uint64_t dwAddress = (uint64_t)pObject;
        DWORD64 dwDisplacement = 0;
        Symbol symbol;
        if (!ms_debugHelp.SymGetSymFromAddr64Fn(
            hProcess,
            dwAddress,
            &dwDisplacement,
            symbol))
        {
            OnError(walker, "SymGetSymFromAddr64", GetLastError(), dwAddress);
            return false;
        }

        OnOutput(walker, symbol.Name);

        return true;
    }
};
