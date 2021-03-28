#include "common/console.h"
#include "allocator/allocator.h"
#include "common/cmd.h"
#include "common/cvar.h"
#include "common/profiler.h"
#include "common/stringutil.h"
#include "containers/strlist.h"
#include "io/fstr.h"
#include "input/input_system.h"
#include "ui/cimgui_ext.h"
#include "rendering/r_window.h"
#include "common/time.h"

#include <string.h>
#include <math.h>
#include <stdarg.h>

#define MAX_LINES       256

static ConVar_t cv_conlogpath =
{
    .type=cvart_text,
    .name="conlogpath",
    .value="console.log",
    .desc="Path to the console log file"
};

static void con_gui(void);
static i32 OnTextInput(ImGuiInputTextCallbackData* data);
static void ExecCmd(const char* cmd, bool history);
static void HistClear(void);

static char ms_buffer[PIM_PATH];
static FStream ms_file;

static i32 ms_iLine;
static char* ms_lines[MAX_LINES];
static u32 ms_colors[MAX_LINES];

static bool ms_autoscroll = true;
static bool ms_scrollToBottom;
static bool ms_showGui;
static bool ms_recapture;

static i32 ms_histCursor;
static StrList ms_history;

void ConSys_Init(void)
{
    ConVar_Reg(&cv_conlogpath);
    StrList_New(&ms_history, EAlloc_Perm);

    ms_file = FStream_Open(cv_conlogpath.value, "wb");
    Con_Clear();
    HistClear();
}

ProfileMark(pm_update, ConSys_Update)
void ConSys_Update(void)
{
    ProfileBegin(pm_update);

    if (ConVar_CheckDirty(&cv_conlogpath))
    {
        FStream_Close(&ms_file);
        ms_file = FStream_Open(cv_conlogpath.value, "wb");
    }
    con_gui();

    ProfileEnd(pm_update);
}

void ConSys_Shutdown(void)
{
    Con_Logf(LogSev_Info, "con", "console shutting down...");
    FStream_Close(&ms_file);

    Con_Clear();
    HistClear();
    StrList_Del(&ms_history);
}

ProfileMark(pm_gui, con_gui)
static void con_gui(void)
{
    bool grabFocus = false;
    if (Input_IsKeyDown(KeyCode_GraveAccent))
    {
        GLFWwindow* focus = Input_GetFocus();
        ms_showGui = !ms_showGui;
        grabFocus = true;
        ms_buffer[0] = 0;
        if (ms_showGui)
        {
            ms_recapture = Input_IsCursorCaptured(focus);
            Input_CaptureCursor(focus, false);
        }
        else
        {
            if (ms_recapture)
            {
                Input_CaptureCursor(focus, true);
            }
            ms_recapture = false;
        }
    }
    if (!ms_showGui)
    {
        return;
    }

    ProfileBegin(pm_gui);

    const u32 winFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoNav;

    ImVec2 size = igGetIO()->DisplaySize;
    size.y *= 0.5f;
    igExSetNextWindowPos((ImVec2) { 0.0f, igGetFrameHeight() }, ImGuiCond_Always);
    igSetNextWindowSize(size, ImGuiCond_Always);
    if (igBegin("Console", &ms_showGui, winFlags))
    {
        if (igSmallButton("Clear"))
        {
            Con_Clear();
        }
        igExSameLine();
        bool logToClipboard = igSmallButton("Copy");
        igExSameLine();
        igCheckbox("AutoScroll", &ms_autoscroll);

        igSeparator();
        const float curHeight = igGetStyle()->ItemSpacing.y + igGetFrameHeightWithSpacing();
        const u32 childFlags = ImGuiWindowFlags_AlwaysVerticalScrollbar;
        igBeginChildStr("ScrollRegion", (ImVec2) { 0.0f, -curHeight }, false, childFlags);
        {
            igPushStyleVarVec2(ImGuiStyleVar_ItemSpacing, (ImVec2) { 4.0f, 1.0f });
            if (logToClipboard)
            {
                igExLogToClipboard();
            }

            const i32 iLine = ms_iLine;
            char const *const *const lines = ms_lines;
            const u32* colors = ms_colors;
            const i32 numLines = NELEM(ms_lines);
            const i32 mask = numLines - 1;

            for (i32 i = 0; i < numLines; ++i)
            {
                const i32 j = (iLine + i) & mask;
                const char* line = lines[j];
                const u32 color = colors[j];
                if (line)
                {
                    igPushStyleColorU32(ImGuiCol_Text, color);
                    igTextUnformatted(line, NULL);
                    igPopStyleColor(1);
                }
            }

            if (logToClipboard)
            {
                igLogFinish();
            }

            if (ms_scrollToBottom || (ms_autoscroll && igGetScrollY() >= igGetScrollMaxY()))
            {
                igSetScrollHereY(1.0f);
            }

            ms_scrollToBottom = false;
            igPopStyleVar(1);
        }
        igEndChild();

        igSeparator();

        const u32 inputFlags =
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_CallbackCompletion |
            ImGuiInputTextFlags_CallbackHistory;
        if (igInputText("", ARGS(ms_buffer), inputFlags, OnTextInput, NULL))
        {
            if (ms_buffer[0])
            {
                Con_Puts(C32_WHITE, ms_buffer);
                ExecCmd(ms_buffer, true);
            }
            ms_buffer[0] = 0;
            grabFocus = true;
            ms_scrollToBottom = true;
        }

        igSetItemDefaultFocus();
        if (grabFocus)
        {
            igSetKeyboardFocusHere(-1);
        }

    }
    igEnd();

    ProfileEnd(pm_gui);
}

void Con_Exec(const char* cmdText)
{
    if (cmdText)
    {
        ExecCmd(cmdText, false);
    }
}

void Con_Puts(u32 color, const char* line)
{
    const i32 numLines = NELEM(ms_lines);
    const i32 mask = numLines - 1;

    ASSERT(line);
    if (line)
    {
        if (FStream_IsOpen(ms_file))
        {
            FStream_Puts(ms_file, line);
        }

        char** lines = ms_lines;
        u32* colors = ms_colors;
        const i32 iLine = ms_iLine++ & mask;

        Mem_Free(lines[iLine]);
        lines[iLine] = StrDup(line, EAlloc_Perm);
        colors[iLine] = color;
    }
}

void Con_Printf(u32 color, const char* fmt, ...)
{
    ASSERT(fmt);
    if (fmt)
    {
        char buffer[4096];
        va_list ap;
        va_start(ap, fmt);
        VSPrintf(ARGS(buffer), fmt, ap);
        va_end(ap);
        Con_Puts(color, buffer);
    }
}

void Con_Clear(void)
{
    ms_iLine = 0;
    for (i32 i = 0; i < NELEM(ms_lines); ++i)
    {
        Mem_Free(ms_lines[i]);
        ms_lines[i] = NULL;
        ms_colors[i] = C32_WHITE;
    }
}

static u32 LogSevToColor(LogSev sev)
{
    switch (sev)
    {
    default:
    case LogSev_Error:
        return C32_RED;
    case LogSev_Warning:
        return C32_YELLOW;
    case LogSev_Info:
        return C32_WHITE;
    case LogSev_Verbose:
        return C32_GRAY;
    }
}

static const char* LogSevToTag(LogSev sev)
{
    switch (sev)
    {
    default:
    case LogSev_Error:
        return "ERROR";
    case LogSev_Warning:
        return "WARN";
    case LogSev_Info:
        return "INFO";
    case LogSev_Verbose:
        return "VERBOSE";
    }
}

void Con_Logf(LogSev sev, const char* tag, const char* fmt, ...)
{
    ASSERT(fmt);
    if (fmt)
    {
        va_list ap;
        va_start(ap, fmt);
        Con_Logv(sev, tag, fmt, ap);
        va_end(ap);
    }
}

void Con_Logv(LogSev sev, const char* tag, const char* fmt, va_list ap)
{
    ASSERT(fmt);
    if (fmt)
    {
        double ms = Time_Milli(Time_Now() - Time_AppStart());
        double seconds = ms / 1000.0;
        ms = fmod(ms, 1000.0);
        double minutes = seconds / 60.0;
        seconds = fmod(seconds, 60.0);
        double hours = minutes / 60.0;
        minutes = fmod(minutes, 60.0);

        u32 sevColor = LogSevToColor(sev);
        const char* sevTag = LogSevToTag(sev);

        char msg[4096];
        SPrintf(ARGS(msg), "[%02d:%02d:%02d:%03d]", (i32)hours, (i32)minutes, (i32)seconds, (i32)ms);
        StrCatf(ARGS(msg), "[%s]", sevTag);
        if (tag)
        {
            StrCatf(ARGS(msg), "[%s]", tag);
        }
        StrCatf(ARGS(msg), " ");

        VStrCatf(ARGS(msg), fmt, ap);

        Con_Puts(sevColor, msg);

        if (sev == LogSev_Error)
        {
            if (FStream_IsOpen(ms_file))
            {
                FStream_Flush(ms_file);
            }
        }
    }
}

static i32 OnTextComplete(ImGuiInputTextCallbackData* data)
{
    char* buffer = data->Buf;
    const i32 capacity = data->BufSize;
    i32 cursor = data->CursorPos;

    while (cursor > 0)
    {
        char prev = buffer[cursor - 1];
        if (prev == ' ' || prev == '\t' || prev == ',' || prev == ';')
        {
            break;
        }
        --cursor;
    }

    char* dst = buffer + cursor;
    const i32 size = capacity - cursor;
    const char* src = cmd_complete(dst);
    if (!src)
    {
        src = ConVar_Complete(dst);
    }
    if (src)
    {
        memset(dst, 0, size);
        StrCpy(dst, size, src);
        data->BufTextLen = StrNLen(buffer, capacity);
        data->CursorPos = data->BufTextLen;
        data->BufDirty = true;
    }

    return 0;
}

static i32 OnTextHistory(ImGuiInputTextCallbackData* data)
{
    const i32 length = ms_history.count;
    if (length)
    {
        i32 cursor = ms_histCursor;
        switch (data->EventKey)
        {
        default:
        case ImGuiKey_UpArrow:
            cursor = (cursor + length - 1) % length;
            break;
        case ImGuiKey_DownArrow:
            cursor = (cursor + 1) % length;
            break;
        }
        char* ptr = ms_history.ptr[cursor];
        if (ptr)
        {
            StrCpy(data->Buf, data->BufSize, ptr);
            data->BufTextLen = StrLen(ptr);
            data->CursorPos = data->BufTextLen;
            data->BufDirty = true;
        }
        ms_histCursor = cursor;
    }
    return 0;
}

static i32 OnTextInput(ImGuiInputTextCallbackData* data)
{
    switch (data->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackCompletion:
        return OnTextComplete(data);
    case ImGuiInputTextFlags_CallbackHistory:
        return OnTextHistory(data);
    }
    return 0;
}

static void HistClear(void)
{
    StrList_Clear(&ms_history);
    ms_histCursor = 0;
}

static void ExecCmd(const char* cmd, bool history)
{
    ASSERT(cmd);

    if (history)
    {
        StrList_Add(&ms_history, cmd);
        ms_histCursor = 0;
    }

    cmd_text(cmd);
}
