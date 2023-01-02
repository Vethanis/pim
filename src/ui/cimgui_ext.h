#pragma once

#include "common/macro.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS 1
#include <cimgui/cimgui.h>

PIM_C_BEGIN

void igExColumns(i32 count);
bool igExTableHeader(i32 count, const char* const* titles, i32* pSelection);
void igExTableFooter(void);
bool igExSliderInt(const char* label, i32* x, i32 lo, i32 hi);
bool igExSliderFloat(const char* label, float* x, float lo, float hi);
bool igExSliderFloat2(const char* label, float x[2], float lo, float hi);
bool igExSliderFloat3(const char* label, float x[3], float lo, float hi);
bool igExSliderFloat4(const char* label, float x[4], float lo, float hi);
bool igExLogSliderInt(const char* label, i32* x, i32 lo, i32 hi);
bool igExLogSliderFloat(const char* label, float* x, float lo, float hi);
bool igExLogSliderFloat2(const char* label, float x[2], float lo, float hi);
bool igExLogSliderFloat3(const char* label, float x[3], float lo, float hi);
bool igExLogSliderFloat4(const char* label, float x[4], float lo, float hi);
bool igExCollapsingHeader1(const char* label);
bool igExCollapsingHeader2(const char* label, bool* pVisible);
void igExSetNextWindowPos(ImVec2 pos, ImGuiCond_ cond);
void igExSameLine(void);
void igExLogToClipboard(void);
bool igExButton(const char* label);
void igExIndent(void);
void igExUnindent(void);

PIM_C_END
