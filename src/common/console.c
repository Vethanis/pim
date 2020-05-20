#include "common/console.h"
#include "allocator/allocator.h"
#include "common/cmd.h"
#include "common/cvar.h"
#include "common/profiler.h"
#include "common/stringutil.h"
#include "common/valist.h"
#include "io/fstr.h"
#include "input/input_system.h"
#include "ui/cimgui.h"
#include "rendering/window.h"

#include <string.h>

#define MAX_LINES       256
#define MAX_HISTORY     64

static cvar_t cv_conlogpath =
{
    cvart_text,
    0x0,
    "conlogpath",
    "console.log",
    "Path to the console log file"
};

static void con_gui(void);
static i32 OnTextInput(ImGuiInputTextCallbackData* data);
static void ExecCmd(const char* cmd);
static void HistClear(void);

static char ms_buffer[PIM_PATH];
static fstr_t ms_file;

static i32 ms_iLine;
static char* ms_lines[MAX_LINES];
static u32 ms_colors[MAX_LINES];

static bool ms_autoscroll = true;
static bool ms_scrollToBottom;
static bool ms_showGui;
static bool ms_recapture;

static i32 ms_iHistory;
static char* ms_history[MAX_HISTORY];

void con_sys_init(void)
{
    cvar_reg(&cv_conlogpath);

    ms_file = fstr_open(cv_conlogpath.value, "wb");
    con_clear();
    HistClear();
}

ProfileMark(pm_update, con_sys_update)
void con_sys_update(void)
{
    ProfileBegin(pm_update);

    if(cvar_check_dirty(&cv_conlogpath))
    {
        fstr_close(&ms_file);
        ms_file = fstr_open(cv_conlogpath.value, "wb");
    }
    con_gui();

    ProfileEnd(pm_update);
}

void con_sys_shutdown(void)
{
    fstr_close(&ms_file);

    con_clear();
    HistClear();
}

ProfileMark(pm_gui, con_gui)
static void con_gui(void)
{
    bool grabFocus = false;
    if (input_keydown(KeyCode_GraveAccent))
    {
        ms_showGui = !ms_showGui;
        grabFocus = true;
        ms_buffer[0] = 0;
        if (ms_showGui)
        {
            ms_recapture = window_cursor_captured();
            window_capture_cursor(false);
        }
        else
        {
            if (ms_recapture)
            {
                window_capture_cursor(true);
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
    igSetNextWindowPos((ImVec2) { 0.0f, igGetFrameHeight() }, ImGuiCond_Always);
    igSetNextWindowSize(size, ImGuiCond_Always);
    if (igBegin("Console", &ms_showGui, winFlags))
    {
        if (igSmallButton("Clear"))
        {
            con_clear();
        }
        igSameLine();
        bool logToClipboard = igSmallButton("Copy");
        igSameLine();
        igCheckbox("AutoScroll", &ms_autoscroll);

        igSeparator();
        const float curHeight = igGetStyle()->ItemSpacing.y + igGetFrameHeightWithSpacing();
        const u32 childFlags = ImGuiWindowFlags_AlwaysVerticalScrollbar;
        igBeginChildStr("ScrollRegion", (ImVec2) { 0.0f, -curHeight }, false, childFlags);
        {
            igPushStyleVarVec2(ImGuiStyleVar_ItemSpacing, (ImVec2) { 4.0f, 1.0f });
            if (logToClipboard)
            {
                igLogToClipboard();
            }

            const i32 iLine = ms_iLine;
            const char** lines = ms_lines;
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
                con_puts(C32_WHITE, ms_buffer);
                ExecCmd(ms_buffer);
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

void con_puts(u32 color, const char* line)
{
    const i32 numLines = NELEM(ms_lines);
    const i32 mask = numLines - 1;

    ASSERT(line);
    if (line)
    {
        if (fstr_isopen(ms_file))
        {
            fstr_puts(ms_file, line);
        }

        char** lines = ms_lines;
        u32* colors = ms_colors;
        const i32 iLine = ms_iLine++ & mask;

        pim_free(lines[iLine]);
        lines[iLine] = StrDup(line, EAlloc_Perm);
        colors[iLine] = color;
    }
}

void con_printf(u32 color, const char* fmt, ...)
{
    ASSERT(fmt);
    if (fmt)
    {
        char buffer[1024];
        VSPrintf(ARGS(buffer), fmt, VA_START(fmt));
        con_puts(color, buffer);
    }
}

void con_clear(void)
{
    u32* colors = ms_colors;
    char** lines = ms_lines;
    const i32 numLines = NELEM(ms_lines);
    for (i32 i = 0; i < numLines; ++i)
    {
        pim_free(lines[i]);
        lines[i] = NULL;
        colors[i] = C32_WHITE;
    }
    ms_iLine = 0;
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
        src = cvar_complete(dst);
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
    const i32 numHist = NELEM(ms_history);
    const i32 mask = numHist - 1;

    i32 iHistory = ms_iHistory;
    switch (data->EventKey)
    {
    default:
    case ImGuiKey_UpArrow:
        iHistory = --iHistory & mask;
    break;
    case ImGuiKey_DownArrow:
        iHistory = ++iHistory & mask;
    break;
    }

    const char* hist = ms_history[iHistory];
    if (hist)
    {
        data->BufTextLen = StrCpy(data->Buf, data->BufSize, hist);
        data->CursorPos = data->BufTextLen;
        data->BufDirty = true;
        ms_iHistory = iHistory;
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
    const i32 len = NELEM(ms_history);
    char** hist = ms_history;
    for (i32 i = 0; i < len; ++i)
    {
        pim_free(hist[i]);
        hist[i] = NULL;
    }
    ms_iHistory = 0;
}

static void ExecCmd(const char* cmd)
{
    ASSERT(cmd);

    const i32 numHist = NELEM(ms_history);
    const i32 mask = numHist - 1;

    i32 slot = ++ms_iHistory & mask;
    pim_free(ms_history[slot]);
    ms_history[slot] = StrDup(cmd, EAlloc_Perm);

    cbuf_t buf;
    cbuf_new(&buf, EAlloc_Perm);
    cbuf_pushback(&buf, cmd);
    cbuf_exec(&buf);
}
