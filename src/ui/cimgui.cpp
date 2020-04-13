#include "ui/imgui.h"
#include "ui/imgui_internal.h"
#include "common/valist.h"

extern "C"
{
ImGuiContext* igCreateContext(ImFontAtlas* shared_font_atlas)
{
    return ImGui::CreateContext(shared_font_atlas);
}
void igDestroyContext(ImGuiContext* ctx)
{
    return ImGui::DestroyContext(ctx);
}
ImGuiContext* igGetCurrentContext()
{
    return ImGui::GetCurrentContext();
}
void igSetCurrentContext(ImGuiContext* ctx)
{
    return ImGui::SetCurrentContext(ctx);
}
bool igDebugCheckVersionAndDataLayout(const char* version_str,usize sz_io,usize sz_style,usize sz_vec2,usize sz_vec4,usize sz_drawvert,usize sz_drawidx)
{
    return ImGui::DebugCheckVersionAndDataLayout(version_str,sz_io,sz_style,sz_vec2,sz_vec4,sz_drawvert,sz_drawidx);
}
ImGuiIO* igGetIO()
{
    return &ImGui::GetIO();
}
ImGuiStyle* igGetStyle()
{
    return &ImGui::GetStyle();
}
void igNewFrame()
{
    return ImGui::NewFrame();
}
void igEndFrame()
{
    return ImGui::EndFrame();
}
void igRender()
{
    return ImGui::Render();
}
ImDrawData* igGetDrawData()
{
    return ImGui::GetDrawData();
}
void igShowDemoWindow(bool* p_open)
{
    return ImGui::ShowDemoWindow(p_open);
}
void igShowAboutWindow(bool* p_open)
{
    return ImGui::ShowAboutWindow(p_open);
}
void igShowMetricsWindow(bool* p_open)
{
    return ImGui::ShowMetricsWindow(p_open);
}
void igShowStyleEditor(ImGuiStyle* ref)
{
    return ImGui::ShowStyleEditor(ref);
}
bool igShowStyleSelector(const char* label)
{
    return ImGui::ShowStyleSelector(label);
}
void igShowFontSelector(const char* label)
{
    return ImGui::ShowFontSelector(label);
}
void igShowUserGuide()
{
    return ImGui::ShowUserGuide();
}
const char* igGetVersion()
{
    return ImGui::GetVersion();
}
void igStyleColorsDark(ImGuiStyle* dst)
{
    return ImGui::StyleColorsDark(dst);
}
void igStyleColorsClassic(ImGuiStyle* dst)
{
    return ImGui::StyleColorsClassic(dst);
}
void igStyleColorsLight(ImGuiStyle* dst)
{
    return ImGui::StyleColorsLight(dst);
}
bool igBegin(const char* name,bool* p_open,ImGuiWindowFlags flags)
{
    return ImGui::Begin(name,p_open,flags);
}
void igEnd()
{
    return ImGui::End();
}
bool igBeginChildStr(const char* str_id,const ImVec2 size,bool border,ImGuiWindowFlags flags)
{
    return ImGui::BeginChild(str_id,size,border,flags);
}
bool igBeginChildID(ImGuiID id,const ImVec2 size,bool border,ImGuiWindowFlags flags)
{
    return ImGui::BeginChild(id,size,border,flags);
}
void igEndChild()
{
    return ImGui::EndChild();
}
bool igIsWindowAppearing()
{
    return ImGui::IsWindowAppearing();
}
bool igIsWindowCollapsed()
{
    return ImGui::IsWindowCollapsed();
}
bool igIsWindowFocused(ImGuiFocusedFlags flags)
{
    return ImGui::IsWindowFocused(flags);
}
bool igIsWindowHovered(ImGuiHoveredFlags flags)
{
    return ImGui::IsWindowHovered(flags);
}
ImDrawList* igGetWindowDrawList()
{
    return ImGui::GetWindowDrawList();
}
void igGetWindowPos(ImVec2 *pOut)
{
    *pOut = ImGui::GetWindowPos();
}
void igGetWindowSize(ImVec2 *pOut)
{
    *pOut = ImGui::GetWindowSize();
}
float igGetWindowWidth()
{
    return ImGui::GetWindowWidth();
}
float igGetWindowHeight()
{
    return ImGui::GetWindowHeight();
}
void igSetNextWindowPos(const ImVec2 pos,ImGuiCond cond,const ImVec2 pivot)
{
    return ImGui::SetNextWindowPos(pos,cond,pivot);
}
void igSetNextWindowSize(const ImVec2 size,ImGuiCond cond)
{
    return ImGui::SetNextWindowSize(size,cond);
}
void igSetNextWindowSizeConstraints(const ImVec2 size_min,const ImVec2 size_max,ImGuiSizeCallback custom_callback,void* custom_callback_data)
{
    return ImGui::SetNextWindowSizeConstraints(size_min,size_max,custom_callback,custom_callback_data);
}
void igSetNextWindowContentSize(const ImVec2 size)
{
    return ImGui::SetNextWindowContentSize(size);
}
void igSetNextWindowCollapsed(bool collapsed,ImGuiCond cond)
{
    return ImGui::SetNextWindowCollapsed(collapsed,cond);
}
void igSetNextWindowFocus()
{
    return ImGui::SetNextWindowFocus();
}
void igSetNextWindowBgAlpha(float alpha)
{
    return ImGui::SetNextWindowBgAlpha(alpha);
}
void igSetWindowPosVec2(const ImVec2 pos,ImGuiCond cond)
{
    return ImGui::SetWindowPos(pos,cond);
}
void igSetWindowSizeVec2(const ImVec2 size,ImGuiCond cond)
{
    return ImGui::SetWindowSize(size,cond);
}
void igSetWindowCollapsedBool(bool collapsed,ImGuiCond cond)
{
    return ImGui::SetWindowCollapsed(collapsed,cond);
}
void igSetWindowFocusNil()
{
    return ImGui::SetWindowFocus();
}
void igSetWindowFontScale(float scale)
{
    return ImGui::SetWindowFontScale(scale);
}
void igSetWindowPosStr(const char* name,const ImVec2 pos,ImGuiCond cond)
{
    return ImGui::SetWindowPos(name,pos,cond);
}
void igSetWindowSizeStr(const char* name,const ImVec2 size,ImGuiCond cond)
{
    return ImGui::SetWindowSize(name,size,cond);
}
void igSetWindowCollapsedStr(const char* name,bool collapsed,ImGuiCond cond)
{
    return ImGui::SetWindowCollapsed(name,collapsed,cond);
}
void igSetWindowFocusStr(const char* name)
{
    return ImGui::SetWindowFocus(name);
}
void igGetContentRegionMax(ImVec2 *pOut)
{
    *pOut = ImGui::GetContentRegionMax();
}
void igGetContentRegionAvail(ImVec2 *pOut)
{
    *pOut = ImGui::GetContentRegionAvail();
}
void igGetWindowContentRegionMin(ImVec2 *pOut)
{
    *pOut = ImGui::GetWindowContentRegionMin();
}
void igGetWindowContentRegionMax(ImVec2 *pOut)
{
    *pOut = ImGui::GetWindowContentRegionMax();
}
float igGetWindowContentRegionWidth()
{
    return ImGui::GetWindowContentRegionWidth();
}
float igGetScrollX()
{
    return ImGui::GetScrollX();
}
float igGetScrollY()
{
    return ImGui::GetScrollY();
}
float igGetScrollMaxX()
{
    return ImGui::GetScrollMaxX();
}
float igGetScrollMaxY()
{
    return ImGui::GetScrollMaxY();
}
void igSetScrollX(float scroll_x)
{
    return ImGui::SetScrollX(scroll_x);
}
void igSetScrollY(float scroll_y)
{
    return ImGui::SetScrollY(scroll_y);
}
void igSetScrollHereX(float center_x_ratio)
{
    return ImGui::SetScrollHereX(center_x_ratio);
}
void igSetScrollHereY(float center_y_ratio)
{
    return ImGui::SetScrollHereY(center_y_ratio);
}
void igSetScrollFromPosX(float local_x,float center_x_ratio)
{
    return ImGui::SetScrollFromPosX(local_x,center_x_ratio);
}
void igSetScrollFromPosY(float local_y,float center_y_ratio)
{
    return ImGui::SetScrollFromPosY(local_y,center_y_ratio);
}
void igPushFont(ImFont* font)
{
    return ImGui::PushFont(font);
}
void igPopFont()
{
    return ImGui::PopFont();
}
void igPushStyleColorU32(ImGuiCol idx,ImU32 col)
{
    return ImGui::PushStyleColor(idx,col);
}
void igPushStyleColorVec4(ImGuiCol idx,const ImVec4 col)
{
    return ImGui::PushStyleColor(idx,col);
}
void igPopStyleColor(int count)
{
    return ImGui::PopStyleColor(count);
}
void igPushStyleVarFloat(ImGuiStyleVar idx,float val)
{
    return ImGui::PushStyleVar(idx,val);
}
void igPushStyleVarVec2(ImGuiStyleVar idx,const ImVec2 val)
{
    return ImGui::PushStyleVar(idx,val);
}
void igPopStyleVar(int count)
{
    return ImGui::PopStyleVar(count);
}
const ImVec4* igGetStyleColorVec4(ImGuiCol idx)
{
    return &ImGui::GetStyleColorVec4(idx);
}
ImFont* igGetFont()
{
    return ImGui::GetFont();
}
float igGetFontSize()
{
    return ImGui::GetFontSize();
}
void igGetFontTexUvWhitePixel(ImVec2 *pOut)
{
    *pOut = ImGui::GetFontTexUvWhitePixel();
}
ImU32 igGetColorU32Col(ImGuiCol idx,float alpha_mul)
{
    return ImGui::GetColorU32(idx,alpha_mul);
}
ImU32 igGetColorU32Vec4(const ImVec4 col)
{
    return ImGui::GetColorU32(col);
}
ImU32 igGetColorU32U32(ImU32 col)
{
    return ImGui::GetColorU32(col);
}
void igPushItemWidth(float item_width)
{
    return ImGui::PushItemWidth(item_width);
}
void igPopItemWidth()
{
    return ImGui::PopItemWidth();
}
void igSetNextItemWidth(float item_width)
{
    return ImGui::SetNextItemWidth(item_width);
}
float igCalcItemWidth()
{
    return ImGui::CalcItemWidth();
}
void igPushTextWrapPos(float wrap_local_pos_x)
{
    return ImGui::PushTextWrapPos(wrap_local_pos_x);
}
void igPopTextWrapPos()
{
    return ImGui::PopTextWrapPos();
}
void igPushAllowKeyboardFocus(bool allow_keyboard_focus)
{
    return ImGui::PushAllowKeyboardFocus(allow_keyboard_focus);
}
void igPopAllowKeyboardFocus()
{
    return ImGui::PopAllowKeyboardFocus();
}
void igPushButtonRepeat(bool repeat)
{
    return ImGui::PushButtonRepeat(repeat);
}
void igPopButtonRepeat()
{
    return ImGui::PopButtonRepeat();
}
void igSeparator()
{
    return ImGui::Separator();
}
void igSameLine(float offset_from_start_x,float spacing)
{
    return ImGui::SameLine(offset_from_start_x,spacing);
}
void igNewLine()
{
    return ImGui::NewLine();
}
void igSpacing()
{
    return ImGui::Spacing();
}
void igDummy(const ImVec2 size)
{
    return ImGui::Dummy(size);
}
void igIndent(float indent_w)
{
    return ImGui::Indent(indent_w);
}
void igUnindent(float indent_w)
{
    return ImGui::Unindent(indent_w);
}
void igBeginGroup()
{
    return ImGui::BeginGroup();
}
void igEndGroup()
{
    return ImGui::EndGroup();
}
void igGetCursorPos(ImVec2 *pOut)
{
    *pOut = ImGui::GetCursorPos();
}
float igGetCursorPosX()
{
    return ImGui::GetCursorPosX();
}
float igGetCursorPosY()
{
    return ImGui::GetCursorPosY();
}
void igSetCursorPos(const ImVec2 local_pos)
{
    return ImGui::SetCursorPos(local_pos);
}
void igSetCursorPosX(float local_x)
{
    return ImGui::SetCursorPosX(local_x);
}
void igSetCursorPosY(float local_y)
{
    return ImGui::SetCursorPosY(local_y);
}
void igGetCursorStartPos(ImVec2 *pOut)
{
    *pOut = ImGui::GetCursorStartPos();
}
void igGetCursorScreenPos(ImVec2 *pOut)
{
    *pOut = ImGui::GetCursorScreenPos();
}
void igSetCursorScreenPos(const ImVec2 pos)
{
    return ImGui::SetCursorScreenPos(pos);
}
void igAlignTextToFramePadding()
{
    return ImGui::AlignTextToFramePadding();
}
float igGetTextLineHeight()
{
    return ImGui::GetTextLineHeight();
}
float igGetTextLineHeightWithSpacing()
{
    return ImGui::GetTextLineHeightWithSpacing();
}
float igGetFrameHeight()
{
    return ImGui::GetFrameHeight();
}
float igGetFrameHeightWithSpacing()
{
    return ImGui::GetFrameHeightWithSpacing();
}
void igPushIDStr(const char* str_id)
{
    return ImGui::PushID(str_id);
}
void igPushIDStrStr(const char* str_id_begin,const char* str_id_end)
{
    return ImGui::PushID(str_id_begin,str_id_end);
}
void igPushIDPtr(const void* ptr_id)
{
    return ImGui::PushID(ptr_id);
}
void igPushIDInt(int int_id)
{
    return ImGui::PushID(int_id);
}
void igPopID()
{
    return ImGui::PopID();
}
ImGuiID igGetIDStr(const char* str_id)
{
    return ImGui::GetID(str_id);
}
ImGuiID igGetIDStrStr(const char* str_id_begin,const char* str_id_end)
{
    return ImGui::GetID(str_id_begin,str_id_end);
}
ImGuiID igGetIDPtr(const void* ptr_id)
{
    return ImGui::GetID(ptr_id);
}
void igTextUnformatted(const char* text,const char* text_end)
{
    return ImGui::TextUnformatted(text,text_end);
}
void igText(const char* fmt,...)
{
    VaList args = VA_START(fmt);
    ImGui::TextV(fmt,args);
}
void igTextV(const char* fmt,VaList args)
{
    return ImGui::TextV(fmt,args);
}
void igTextColored(const ImVec4 col,const char* fmt,...)
{
    VaList args = VA_START(fmt);
    ImGui::TextColoredV(col,fmt,args);
}
void igTextColoredV(const ImVec4 col,const char* fmt,VaList args)
{
    return ImGui::TextColoredV(col,fmt,args);
}
void igTextDisabled(const char* fmt,...)
{
    VaList args = VA_START(fmt);
    ImGui::TextDisabledV(fmt,args);
}
void igTextDisabledV(const char* fmt,VaList args)
{
    return ImGui::TextDisabledV(fmt,args);
}
void igTextWrapped(const char* fmt,...)
{
    VaList args = VA_START(fmt);
    ImGui::TextWrappedV(fmt,args);
}
void igTextWrappedV(const char* fmt,VaList args)
{
    return ImGui::TextWrappedV(fmt,args);
}
void igLabelText(const char* label,const char* fmt,...)
{
    VaList args = VA_START(fmt);
    ImGui::LabelTextV(label,fmt,args);
}
void igLabelTextV(const char* label,const char* fmt,VaList args)
{
    return ImGui::LabelTextV(label,fmt,args);
}
void igBulletText(const char* fmt,...)
{
    VaList args = VA_START(fmt);
    ImGui::BulletTextV(fmt,args);
}
void igBulletTextV(const char* fmt,VaList args)
{
    return ImGui::BulletTextV(fmt,args);
}
bool igButton(const char* label)
{
    return ImGui::Button(label);
}
bool igSmallButton(const char* label)
{
    return ImGui::SmallButton(label);
}
bool igInvisibleButton(const char* str_id,const ImVec2 size)
{
    return ImGui::InvisibleButton(str_id,size);
}
bool igArrowButton(const char* str_id,ImGuiDir dir)
{
    return ImGui::ArrowButton(str_id,dir);
}
void igImage(ImTextureID user_texture_id,const ImVec2 size,const ImVec2 uv0,const ImVec2 uv1,const ImVec4 tint_col,const ImVec4 border_col)
{
    return ImGui::Image(user_texture_id,size,uv0,uv1,tint_col,border_col);
}
bool igImageButton(ImTextureID user_texture_id,const ImVec2 size,const ImVec2 uv0,const ImVec2 uv1,int frame_padding,const ImVec4 bg_col,const ImVec4 tint_col)
{
    return ImGui::ImageButton(user_texture_id,size,uv0,uv1,frame_padding,bg_col,tint_col);
}
bool igCheckbox(const char* label,bool* v)
{
    return ImGui::Checkbox(label,v);
}
bool igCheckboxFlags(const char* label,unsigned int* flags,unsigned int flags_value)
{
    return ImGui::CheckboxFlags(label,flags,flags_value);
}
bool igRadioButtonBool(const char* label,bool active)
{
    return ImGui::RadioButton(label,active);
}
bool igRadioButtonIntPtr(const char* label,int* v,int v_button)
{
    return ImGui::RadioButton(label,v,v_button);
}
void igProgressBar(float fraction,const ImVec2 size_arg,const char* overlay)
{
    return ImGui::ProgressBar(fraction,size_arg,overlay);
}
void igBullet()
{
    return ImGui::Bullet();
}
bool igBeginCombo(const char* label,const char* preview_value,ImGuiComboFlags flags)
{
    return ImGui::BeginCombo(label,preview_value,flags);
}
void igEndCombo()
{
    return ImGui::EndCombo();
}
bool igComboStr_arr(const char* label,int* current_item,const char* const items[],int items_count,int popup_max_height_in_items)
{
    return ImGui::Combo(label,current_item,items,items_count,popup_max_height_in_items);
}
bool igComboStr(const char* label,int* current_item,const char* items_separated_by_zeros,int popup_max_height_in_items)
{
    return ImGui::Combo(label,current_item,items_separated_by_zeros,popup_max_height_in_items);
}
bool igComboFnPtr(const char* label,int* current_item,bool(*items_getter)(void* data,int idx,const char** out_text),void* data,int items_count,int popup_max_height_in_items)
{
    return ImGui::Combo(label,current_item,items_getter,data,items_count,popup_max_height_in_items);
}
bool igDragFloat(const char* label,float* v,float v_speed,float v_min,float v_max,const char* format,float power)
{
    return ImGui::DragFloat(label,v,v_speed,v_min,v_max,format,power);
}
bool igDragFloat2(const char* label,float v[2],float v_speed,float v_min,float v_max,const char* format,float power)
{
    return ImGui::DragFloat2(label,v,v_speed,v_min,v_max,format,power);
}
bool igDragFloat3(const char* label,float v[3],float v_speed,float v_min,float v_max,const char* format,float power)
{
    return ImGui::DragFloat3(label,v,v_speed,v_min,v_max,format,power);
}
bool igDragFloat4(const char* label,float v[4],float v_speed,float v_min,float v_max,const char* format,float power)
{
    return ImGui::DragFloat4(label,v,v_speed,v_min,v_max,format,power);
}
bool igDragFloatRange2(const char* label,float* v_current_min,float* v_current_max,float v_speed,float v_min,float v_max,const char* format,const char* format_max,float power)
{
    return ImGui::DragFloatRange2(label,v_current_min,v_current_max,v_speed,v_min,v_max,format,format_max,power);
}
bool igDragInt(const char* label,int* v,float v_speed,int v_min,int v_max,const char* format)
{
    return ImGui::DragInt(label,v,v_speed,v_min,v_max,format);
}
bool igDragInt2(const char* label,int v[2],float v_speed,int v_min,int v_max,const char* format)
{
    return ImGui::DragInt2(label,v,v_speed,v_min,v_max,format);
}
bool igDragInt3(const char* label,int v[3],float v_speed,int v_min,int v_max,const char* format)
{
    return ImGui::DragInt3(label,v,v_speed,v_min,v_max,format);
}
bool igDragInt4(const char* label,int v[4],float v_speed,int v_min,int v_max,const char* format)
{
    return ImGui::DragInt4(label,v,v_speed,v_min,v_max,format);
}
bool igDragIntRange2(const char* label,int* v_current_min,int* v_current_max,float v_speed,int v_min,int v_max,const char* format,const char* format_max)
{
    return ImGui::DragIntRange2(label,v_current_min,v_current_max,v_speed,v_min,v_max,format,format_max);
}
bool igDragScalar(const char* label,ImGuiDataType data_type,void* p_data,float v_speed,const void* p_min,const void* p_max,const char* format,float power)
{
    return ImGui::DragScalar(label,data_type,p_data,v_speed,p_min,p_max,format,power);
}
bool igDragScalarN(const char* label,ImGuiDataType data_type,void* p_data,int components,float v_speed,const void* p_min,const void* p_max,const char* format,float power)
{
    return ImGui::DragScalarN(label,data_type,p_data,components,v_speed,p_min,p_max,format,power);
}
bool igSliderFloat(const char* label,float* v,float v_min,float v_max,const char* format,float power)
{
    return ImGui::SliderFloat(label,v,v_min,v_max,format,power);
}
bool igSliderFloat2(const char* label,float v[2],float v_min,float v_max,const char* format,float power)
{
    return ImGui::SliderFloat2(label,v,v_min,v_max,format,power);
}
bool igSliderFloat3(const char* label,float v[3],float v_min,float v_max,const char* format,float power)
{
    return ImGui::SliderFloat3(label,v,v_min,v_max,format,power);
}
bool igSliderFloat4(const char* label,float v[4],float v_min,float v_max,const char* format,float power)
{
    return ImGui::SliderFloat4(label,v,v_min,v_max,format,power);
}
bool igSliderAngle(const char* label,float* v_rad,float v_degrees_min,float v_degrees_max,const char* format)
{
    return ImGui::SliderAngle(label,v_rad,v_degrees_min,v_degrees_max,format);
}
bool igSliderInt(const char* label,int* v,int v_min,int v_max,const char* format)
{
    return ImGui::SliderInt(label,v,v_min,v_max,format);
}
bool igSliderInt2(const char* label,int v[2],int v_min,int v_max,const char* format)
{
    return ImGui::SliderInt2(label,v,v_min,v_max,format);
}
bool igSliderInt3(const char* label,int v[3],int v_min,int v_max,const char* format)
{
    return ImGui::SliderInt3(label,v,v_min,v_max,format);
}
bool igSliderInt4(const char* label,int v[4],int v_min,int v_max,const char* format)
{
    return ImGui::SliderInt4(label,v,v_min,v_max,format);
}
bool igSliderScalar(const char* label,ImGuiDataType data_type,void* p_data,const void* p_min,const void* p_max,const char* format,float power)
{
    return ImGui::SliderScalar(label,data_type,p_data,p_min,p_max,format,power);
}
bool igSliderScalarN(const char* label,ImGuiDataType data_type,void* p_data,int components,const void* p_min,const void* p_max,const char* format,float power)
{
    return ImGui::SliderScalarN(label,data_type,p_data,components,p_min,p_max,format,power);
}
bool igVSliderFloat(const char* label,const ImVec2 size,float* v,float v_min,float v_max,const char* format,float power)
{
    return ImGui::VSliderFloat(label,size,v,v_min,v_max,format,power);
}
bool igVSliderInt(const char* label,const ImVec2 size,int* v,int v_min,int v_max,const char* format)
{
    return ImGui::VSliderInt(label,size,v,v_min,v_max,format);
}
bool igVSliderScalar(const char* label,const ImVec2 size,ImGuiDataType data_type,void* p_data,const void* p_min,const void* p_max,const char* format,float power)
{
    return ImGui::VSliderScalar(label,size,data_type,p_data,p_min,p_max,format,power);
}
bool igInputText(const char* label,char* buf,usize buf_size,ImGuiInputTextFlags flags,ImGuiInputTextCallback callback,void* user_data)
{
    return ImGui::InputText(label,buf,buf_size,flags,callback,user_data);
}
bool igInputTextMultiline(const char* label,char* buf,usize buf_size,const ImVec2 size,ImGuiInputTextFlags flags,ImGuiInputTextCallback callback,void* user_data)
{
    return ImGui::InputTextMultiline(label,buf,buf_size,size,flags,callback,user_data);
}
bool igInputTextWithHint(const char* label,const char* hint,char* buf,usize buf_size,ImGuiInputTextFlags flags,ImGuiInputTextCallback callback,void* user_data)
{
    return ImGui::InputTextWithHint(label,hint,buf,buf_size,flags,callback,user_data);
}
bool igInputFloat(const char* label,float* v,float step,float step_fast,const char* format,ImGuiInputTextFlags flags)
{
    return ImGui::InputFloat(label,v,step,step_fast,format,flags);
}
bool igInputFloat2(const char* label,float v[2],const char* format,ImGuiInputTextFlags flags)
{
    return ImGui::InputFloat2(label,v,format,flags);
}
bool igInputFloat3(const char* label,float v[3],const char* format,ImGuiInputTextFlags flags)
{
    return ImGui::InputFloat3(label,v,format,flags);
}
bool igInputFloat4(const char* label,float v[4],const char* format,ImGuiInputTextFlags flags)
{
    return ImGui::InputFloat4(label,v,format,flags);
}
bool igInputInt(const char* label,int* v,int step,int step_fast,ImGuiInputTextFlags flags)
{
    return ImGui::InputInt(label,v,step,step_fast,flags);
}
bool igInputInt2(const char* label,int v[2],ImGuiInputTextFlags flags)
{
    return ImGui::InputInt2(label,v,flags);
}
bool igInputInt3(const char* label,int v[3],ImGuiInputTextFlags flags)
{
    return ImGui::InputInt3(label,v,flags);
}
bool igInputInt4(const char* label,int v[4],ImGuiInputTextFlags flags)
{
    return ImGui::InputInt4(label,v,flags);
}
bool igInputDouble(const char* label,double* v,double step,double step_fast,const char* format,ImGuiInputTextFlags flags)
{
    return ImGui::InputDouble(label,v,step,step_fast,format,flags);
}
bool igInputScalar(const char* label,ImGuiDataType data_type,void* p_data,const void* p_step,const void* p_step_fast,const char* format,ImGuiInputTextFlags flags)
{
    return ImGui::InputScalar(label,data_type,p_data,p_step,p_step_fast,format,flags);
}
bool igInputScalarN(const char* label,ImGuiDataType data_type,void* p_data,int components,const void* p_step,const void* p_step_fast,const char* format,ImGuiInputTextFlags flags)
{
    return ImGui::InputScalarN(label,data_type,p_data,components,p_step,p_step_fast,format,flags);
}
bool igColorEdit3(const char* label,float col[3],ImGuiColorEditFlags flags)
{
    return ImGui::ColorEdit3(label,col,flags);
}
bool igColorEdit4(const char* label,float col[4],ImGuiColorEditFlags flags)
{
    return ImGui::ColorEdit4(label,col,flags);
}
bool igColorPicker3(const char* label,float col[3],ImGuiColorEditFlags flags)
{
    return ImGui::ColorPicker3(label,col,flags);
}
bool igColorPicker4(const char* label,float col[4],ImGuiColorEditFlags flags,const float* ref_col)
{
    return ImGui::ColorPicker4(label,col,flags,ref_col);
}
bool igColorButton(const char* desc_id,const ImVec4 col,ImGuiColorEditFlags flags,ImVec2 size)
{
    return ImGui::ColorButton(desc_id,col,flags,size);
}
void igSetColorEditOptions(ImGuiColorEditFlags flags)
{
    return ImGui::SetColorEditOptions(flags);
}
bool igTreeNodeStr(const char* label)
{
    return ImGui::TreeNode(label);
}
bool igTreeNodeStrStr(const char* str_id,const char* fmt,...)
{
    VaList args = VA_START(fmt);
    bool ret = ImGui::TreeNodeV(str_id,fmt,args);
    return ret;
}
bool igTreeNodePtr(const void* ptr_id,const char* fmt,...)
{
    VaList args = VA_START(fmt);
    bool ret = ImGui::TreeNodeV(ptr_id,fmt,args);
    return ret;
}
bool igTreeNodeVStr(const char* str_id,const char* fmt,VaList args)
{
    return ImGui::TreeNodeV(str_id,fmt,args);
}
bool igTreeNodeVPtr(const void* ptr_id,const char* fmt,VaList args)
{
    return ImGui::TreeNodeV(ptr_id,fmt,args);
}
bool igTreeNodeExStr(const char* label,ImGuiTreeNodeFlags flags)
{
    return ImGui::TreeNodeEx(label,flags);
}
bool igTreeNodeExStrStr(const char* str_id,ImGuiTreeNodeFlags flags,const char* fmt,...)
{
    VaList args = VA_START(fmt);
    bool ret = ImGui::TreeNodeExV(str_id,flags,fmt,args);
    return ret;
}
bool igTreeNodeExPtr(const void* ptr_id,ImGuiTreeNodeFlags flags,const char* fmt,...)
{
    VaList args = VA_START(fmt);
    bool ret = ImGui::TreeNodeExV(ptr_id,flags,fmt,args);
    return ret;
}
bool igTreeNodeExVStr(const char* str_id,ImGuiTreeNodeFlags flags,const char* fmt,VaList args)
{
    return ImGui::TreeNodeExV(str_id,flags,fmt,args);
}
bool igTreeNodeExVPtr(const void* ptr_id,ImGuiTreeNodeFlags flags,const char* fmt,VaList args)
{
    return ImGui::TreeNodeExV(ptr_id,flags,fmt,args);
}
void igTreePushStr(const char* str_id)
{
    return ImGui::TreePush(str_id);
}
void igTreePushPtr(const void* ptr_id)
{
    return ImGui::TreePush(ptr_id);
}
void igTreePop()
{
    return ImGui::TreePop();
}
float igGetTreeNodeToLabelSpacing()
{
    return ImGui::GetTreeNodeToLabelSpacing();
}
bool igCollapsingHeader1(const char* label)
{
    return ImGui::CollapsingHeader(label);
}
bool igCollapsingHeader2(const char* label,bool* p_open)
{
    return ImGui::CollapsingHeader(label,p_open);
}
void igSetNextItemOpen(bool is_open,ImGuiCond cond)
{
    return ImGui::SetNextItemOpen(is_open,cond);
}
bool igSelectableBool(const char* label,bool selected,ImGuiSelectableFlags flags,const ImVec2 size)
{
    return ImGui::Selectable(label,selected,flags,size);
}
bool igSelectableBoolPtr(const char* label,bool* p_selected,ImGuiSelectableFlags flags,const ImVec2 size)
{
    return ImGui::Selectable(label,p_selected,flags,size);
}
bool igListBoxStr_arr(const char* label,int* current_item,const char* const items[],int items_count,int height_in_items)
{
    return ImGui::ListBox(label,current_item,items,items_count,height_in_items);
}
bool igListBoxFnPtr(const char* label,int* current_item,bool(*items_getter)(void* data,int idx,const char** out_text),void* data,int items_count,int height_in_items)
{
    return ImGui::ListBox(label,current_item,items_getter,data,items_count,height_in_items);
}
bool igListBoxHeaderVec2(const char* label,const ImVec2 size)
{
    return ImGui::ListBoxHeader(label,size);
}
bool igListBoxHeaderInt(const char* label,int items_count,int height_in_items)
{
    return ImGui::ListBoxHeader(label,items_count,height_in_items);
}
void igListBoxFooter()
{
    return ImGui::ListBoxFooter();
}
void igPlotLinesFloatPtr(const char* label,const float* values,int values_count,int values_offset,const char* overlay_text,float scale_min,float scale_max,ImVec2 graph_size,int stride)
{
    return ImGui::PlotLines(label,values,values_count,values_offset,overlay_text,scale_min,scale_max,graph_size,stride);
}
void igPlotLinesFnPtr(const char* label,float(*values_getter)(void* data,int idx),void* data,int values_count,int values_offset,const char* overlay_text,float scale_min,float scale_max,ImVec2 graph_size)
{
    return ImGui::PlotLines(label,values_getter,data,values_count,values_offset,overlay_text,scale_min,scale_max,graph_size);
}
void igPlotHistogramFloatPtr(const char* label,const float* values,int values_count,int values_offset,const char* overlay_text,float scale_min,float scale_max,ImVec2 graph_size,int stride)
{
    return ImGui::PlotHistogram(label,values,values_count,values_offset,overlay_text,scale_min,scale_max,graph_size,stride);
}
void igPlotHistogramFnPtr(const char* label,float(*values_getter)(void* data,int idx),void* data,int values_count,int values_offset,const char* overlay_text,float scale_min,float scale_max,ImVec2 graph_size)
{
    return ImGui::PlotHistogram(label,values_getter,data,values_count,values_offset,overlay_text,scale_min,scale_max,graph_size);
}
void igValueBool(const char* prefix,bool b)
{
    return ImGui::Value(prefix,b);
}
void igValueInt(const char* prefix,int v)
{
    return ImGui::Value(prefix,v);
}
void igValueUint(const char* prefix,unsigned int v)
{
    return ImGui::Value(prefix,v);
}
void igValueFloat(const char* prefix,float v,const char* float_format)
{
    return ImGui::Value(prefix,v,float_format);
}
bool igBeginMenuBar()
{
    return ImGui::BeginMenuBar();
}
void igEndMenuBar()
{
    return ImGui::EndMenuBar();
}
bool igBeginMainMenuBar()
{
    return ImGui::BeginMainMenuBar();
}
void igEndMainMenuBar()
{
    return ImGui::EndMainMenuBar();
}
bool igBeginMenu(const char* label,bool enabled)
{
    return ImGui::BeginMenu(label,enabled);
}
void igEndMenu()
{
    return ImGui::EndMenu();
}
bool igMenuItemBool(const char* label,const char* shortcut,bool selected,bool enabled)
{
    return ImGui::MenuItem(label,shortcut,selected,enabled);
}
bool igMenuItemBoolPtr(const char* label,const char* shortcut,bool* p_selected,bool enabled)
{
    return ImGui::MenuItem(label,shortcut,p_selected,enabled);
}
void igBeginTooltip()
{
    return ImGui::BeginTooltip();
}
void igEndTooltip()
{
    return ImGui::EndTooltip();
}
void igSetTooltip(const char* fmt,...)
{
    VaList args = VA_START(fmt);
    ImGui::SetTooltipV(fmt,args);
}
void igSetTooltipV(const char* fmt,VaList args)
{
    return ImGui::SetTooltipV(fmt,args);
}
void igOpenPopup(const char* str_id)
{
    return ImGui::OpenPopup(str_id);
}
bool igBeginPopup(const char* str_id,ImGuiWindowFlags flags)
{
    return ImGui::BeginPopup(str_id,flags);
}
bool igBeginPopupContextItem(const char* str_id,ImGuiMouseButton mouse_button)
{
    return ImGui::BeginPopupContextItem(str_id,mouse_button);
}
bool igBeginPopupContextWindow(const char* str_id,ImGuiMouseButton mouse_button,bool also_over_items)
{
    return ImGui::BeginPopupContextWindow(str_id,mouse_button,also_over_items);
}
bool igBeginPopupContextVoid(const char* str_id,ImGuiMouseButton mouse_button)
{
    return ImGui::BeginPopupContextVoid(str_id,mouse_button);
}
bool igBeginPopupModal(const char* name,bool* p_open,ImGuiWindowFlags flags)
{
    return ImGui::BeginPopupModal(name,p_open,flags);
}
void igEndPopup()
{
    return ImGui::EndPopup();
}
bool igOpenPopupOnItemClick(const char* str_id,ImGuiMouseButton mouse_button)
{
    return ImGui::OpenPopupOnItemClick(str_id,mouse_button);
}
bool igIsPopupOpen(const char* str_id)
{
    return ImGui::IsPopupOpen(str_id);
}
void igCloseCurrentPopup()
{
    return ImGui::CloseCurrentPopup();
}
void igColumns(int count,const char* id,bool border)
{
    return ImGui::Columns(count,id,border);
}
void igNextColumn()
{
    return ImGui::NextColumn();
}
int igGetColumnIndex()
{
    return ImGui::GetColumnIndex();
}
float igGetColumnWidth(int column_index)
{
    return ImGui::GetColumnWidth(column_index);
}
void igSetColumnWidth(int column_index,float width)
{
    return ImGui::SetColumnWidth(column_index,width);
}
float igGetColumnOffset(int column_index)
{
    return ImGui::GetColumnOffset(column_index);
}
void igSetColumnOffset(int column_index,float offset_x)
{
    return ImGui::SetColumnOffset(column_index,offset_x);
}
int igGetColumnsCount()
{
    return ImGui::GetColumnsCount();
}
bool igBeginTabBar(const char* str_id,ImGuiTabBarFlags flags)
{
    return ImGui::BeginTabBar(str_id,flags);
}
void igEndTabBar()
{
    return ImGui::EndTabBar();
}
bool igBeginTabItem(const char* label,bool* p_open,ImGuiTabItemFlags flags)
{
    return ImGui::BeginTabItem(label,p_open,flags);
}
void igEndTabItem()
{
    return ImGui::EndTabItem();
}
void igSetTabItemClosed(const char* tab_or_docked_window_label)
{
    return ImGui::SetTabItemClosed(tab_or_docked_window_label);
}
void igLogToTTY(int auto_open_depth)
{
    return ImGui::LogToTTY(auto_open_depth);
}
void igLogToFile(int auto_open_depth,const char* filename)
{
    return ImGui::LogToFile(auto_open_depth,filename);
}
void igLogToClipboard(int auto_open_depth)
{
    return ImGui::LogToClipboard(auto_open_depth);
}
void igLogFinish()
{
    return ImGui::LogFinish();
}
void igLogButtons()
{
    return ImGui::LogButtons();
}
bool igBeginDragDropSource(ImGuiDragDropFlags flags)
{
    return ImGui::BeginDragDropSource(flags);
}
bool igSetDragDropPayload(const char* type,const void* data,usize sz,ImGuiCond cond)
{
    return ImGui::SetDragDropPayload(type,data,sz,cond);
}
void igEndDragDropSource()
{
    return ImGui::EndDragDropSource();
}
bool igBeginDragDropTarget()
{
    return ImGui::BeginDragDropTarget();
}
const ImGuiPayload* igAcceptDragDropPayload(const char* type,ImGuiDragDropFlags flags)
{
    return ImGui::AcceptDragDropPayload(type,flags);
}
void igEndDragDropTarget()
{
    return ImGui::EndDragDropTarget();
}
const ImGuiPayload* igGetDragDropPayload()
{
    return ImGui::GetDragDropPayload();
}
void igPushClipRect(const ImVec2 clip_rect_min,const ImVec2 clip_rect_max,bool intersect_with_current_clip_rect)
{
    return ImGui::PushClipRect(clip_rect_min,clip_rect_max,intersect_with_current_clip_rect);
}
void igPopClipRect()
{
    return ImGui::PopClipRect();
}
void igSetItemDefaultFocus()
{
    return ImGui::SetItemDefaultFocus();
}
void igSetKeyboardFocusHere(int offset)
{
    return ImGui::SetKeyboardFocusHere(offset);
}
bool igIsItemHovered(ImGuiHoveredFlags flags)
{
    return ImGui::IsItemHovered(flags);
}
bool igIsItemActive()
{
    return ImGui::IsItemActive();
}
bool igIsItemFocused()
{
    return ImGui::IsItemFocused();
}
bool igIsItemClicked(ImGuiMouseButton mouse_button)
{
    return ImGui::IsItemClicked(mouse_button);
}
bool igIsItemVisible()
{
    return ImGui::IsItemVisible();
}
bool igIsItemEdited()
{
    return ImGui::IsItemEdited();
}
bool igIsItemActivated()
{
    return ImGui::IsItemActivated();
}
bool igIsItemDeactivated()
{
    return ImGui::IsItemDeactivated();
}
bool igIsItemDeactivatedAfterEdit()
{
    return ImGui::IsItemDeactivatedAfterEdit();
}
bool igIsItemToggledOpen()
{
    return ImGui::IsItemToggledOpen();
}
bool igIsAnyItemHovered()
{
    return ImGui::IsAnyItemHovered();
}
bool igIsAnyItemActive()
{
    return ImGui::IsAnyItemActive();
}
bool igIsAnyItemFocused()
{
    return ImGui::IsAnyItemFocused();
}
void igGetItemRectMin(ImVec2 *pOut)
{
    *pOut = ImGui::GetItemRectMin();
}
void igGetItemRectMax(ImVec2 *pOut)
{
    *pOut = ImGui::GetItemRectMax();
}
void igGetItemRectSize(ImVec2 *pOut)
{
    *pOut = ImGui::GetItemRectSize();
}
void igSetItemAllowOverlap()
{
    return ImGui::SetItemAllowOverlap();
}
bool igIsRectVisibleNil(const ImVec2 size)
{
    return ImGui::IsRectVisible(size);
}
bool igIsRectVisibleVec2(const ImVec2 rect_min,const ImVec2 rect_max)
{
    return ImGui::IsRectVisible(rect_min,rect_max);
}
double igGetTime()
{
    return ImGui::GetTime();
}
int igGetFrameCount()
{
    return ImGui::GetFrameCount();
}
ImDrawList* igGetBackgroundDrawList()
{
    return ImGui::GetBackgroundDrawList();
}
ImDrawList* igGetForegroundDrawList()
{
    return ImGui::GetForegroundDrawList();
}
ImDrawListSharedData* igGetDrawListSharedData()
{
    return ImGui::GetDrawListSharedData();
}
const char* igGetStyleColorName(ImGuiCol idx)
{
    return ImGui::GetStyleColorName(idx);
}
void igSetStateStorage(ImGuiStorage* storage)
{
    return ImGui::SetStateStorage(storage);
}
ImGuiStorage* igGetStateStorage()
{
    return ImGui::GetStateStorage();
}
void igCalcTextSize(ImVec2 *pOut,const char* text,const char* text_end,bool hide_text_after_double_hash,float wrap_width)
{
    *pOut = ImGui::CalcTextSize(text,text_end,hide_text_after_double_hash,wrap_width);
}
void igCalcListClipping(int items_count,float items_height,int* out_items_display_start,int* out_items_display_end)
{
    return ImGui::CalcListClipping(items_count,items_height,out_items_display_start,out_items_display_end);
}
bool igBeginChildFrame(ImGuiID id,const ImVec2 size,ImGuiWindowFlags flags)
{
    return ImGui::BeginChildFrame(id,size,flags);
}
void igEndChildFrame()
{
    return ImGui::EndChildFrame();
}
void igColorConvertU32ToFloat4(ImVec4 *pOut,ImU32 in)
{
    *pOut = ImGui::ColorConvertU32ToFloat4(in);
}
ImU32 igColorConvertFloat4ToU32(const ImVec4 in)
{
    return ImGui::ColorConvertFloat4ToU32(in);
}
int igGetKeyIndex(ImGuiKey imgui_key)
{
    return ImGui::GetKeyIndex(imgui_key);
}
bool igIsKeyDown(int user_key_index)
{
    return ImGui::IsKeyDown(user_key_index);
}
bool igIsKeyPressed(int user_key_index,bool repeat)
{
    return ImGui::IsKeyPressed(user_key_index,repeat);
}
bool igIsKeyReleased(int user_key_index)
{
    return ImGui::IsKeyReleased(user_key_index);
}
int igGetKeyPressedAmount(int key_index,float repeat_delay,float rate)
{
    return ImGui::GetKeyPressedAmount(key_index,repeat_delay,rate);
}
void igCaptureKeyboardFromApp(bool want_capture_keyboard_value)
{
    return ImGui::CaptureKeyboardFromApp(want_capture_keyboard_value);
}
bool igIsMouseDown(ImGuiMouseButton button)
{
    return ImGui::IsMouseDown(button);
}
bool igIsMouseClicked(ImGuiMouseButton button,bool repeat)
{
    return ImGui::IsMouseClicked(button,repeat);
}
bool igIsMouseReleased(ImGuiMouseButton button)
{
    return ImGui::IsMouseReleased(button);
}
bool igIsMouseDoubleClicked(ImGuiMouseButton button)
{
    return ImGui::IsMouseDoubleClicked(button);
}
bool igIsMouseHoveringRect(const ImVec2 r_min,const ImVec2 r_max,bool clip)
{
    return ImGui::IsMouseHoveringRect(r_min,r_max,clip);
}
bool igIsMousePosValid(const ImVec2* mouse_pos)
{
    return ImGui::IsMousePosValid(mouse_pos);
}
bool igIsAnyMouseDown()
{
    return ImGui::IsAnyMouseDown();
}
void igGetMousePos(ImVec2 *pOut)
{
    *pOut = ImGui::GetMousePos();
}
void igGetMousePosOnOpeningCurrentPopup(ImVec2 *pOut)
{
    *pOut = ImGui::GetMousePosOnOpeningCurrentPopup();
}
bool igIsMouseDragging(ImGuiMouseButton button,float lock_threshold)
{
    return ImGui::IsMouseDragging(button,lock_threshold);
}
void igGetMouseDragDelta(ImVec2 *pOut,ImGuiMouseButton button,float lock_threshold)
{
    *pOut = ImGui::GetMouseDragDelta(button,lock_threshold);
}
void igResetMouseDragDelta(ImGuiMouseButton button)
{
    return ImGui::ResetMouseDragDelta(button);
}
ImGuiMouseCursor igGetMouseCursor()
{
    return ImGui::GetMouseCursor();
}
void igSetMouseCursor(ImGuiMouseCursor cursor_type)
{
    return ImGui::SetMouseCursor(cursor_type);
}
void igCaptureMouseFromApp(bool want_capture_mouse_value)
{
    return ImGui::CaptureMouseFromApp(want_capture_mouse_value);
}
const char* igGetClipboardText()
{
    return ImGui::GetClipboardText();
}
void igSetClipboardText(const char* text)
{
    return ImGui::SetClipboardText(text);
}
void igLoadIniSettingsFromDisk(const char* ini_filename)
{
    return ImGui::LoadIniSettingsFromDisk(ini_filename);
}
void igLoadIniSettingsFromMemory(const char* ini_data,usize ini_size)
{
    return ImGui::LoadIniSettingsFromMemory(ini_data,ini_size);
}
void igSaveIniSettingsToDisk(const char* ini_filename)
{
    return ImGui::SaveIniSettingsToDisk(ini_filename);
}
const char* igSaveIniSettingsToMemory(usize* out_ini_size)
{
    return ImGui::SaveIniSettingsToMemory(out_ini_size);
}
void igSetAllocatorFunctions(void*(*alloc_func)(usize sz,void* user_data),void(*free_func)(void* ptr,void* user_data),void* user_data)
{
    return ImGui::SetAllocatorFunctions(alloc_func,free_func,user_data);
}
void* igMemAlloc(usize size)
{
    return ImGui::MemAlloc(size);
}
void igMemFree(void* ptr)
{
    return ImGui::MemFree(ptr);
}
void ImGuiStyle_ScaleAllSizes(ImGuiStyle* self,float scale_factor)
{
    return self->ScaleAllSizes(scale_factor);
}
void ImGuiIO_AddInputCharacter(ImGuiIO* self,unsigned int c)
{
    return self->AddInputCharacter(c);
}
void ImGuiIO_AddInputCharactersUTF8(ImGuiIO* self,const char* str)
{
    return self->AddInputCharactersUTF8(str);
}
void ImGuiIO_ClearInputCharacters(ImGuiIO* self)
{
    return self->ClearInputCharacters();
}
void ImGuiInputTextCallbackData_DeleteChars(ImGuiInputTextCallbackData* self,int pos,int bytes_count)
{
    return self->DeleteChars(pos,bytes_count);
}
void ImGuiInputTextCallbackData_InsertChars(ImGuiInputTextCallbackData* self,int pos,const char* text,const char* text_end)
{
    return self->InsertChars(pos,text,text_end);
}
bool ImGuiInputTextCallbackData_HasSelection(ImGuiInputTextCallbackData* self)
{
    return self->HasSelection();
}
void ImGuiPayload_Clear(ImGuiPayload* self)
{
    return self->Clear();
}
bool ImGuiPayload_IsDataType(ImGuiPayload* self,const char* type)
{
    return self->IsDataType(type);
}
bool ImGuiPayload_IsPreview(ImGuiPayload* self)
{
    return self->IsPreview();
}
bool ImGuiPayload_IsDelivery(ImGuiPayload* self)
{
    return self->IsDelivery();
}
void ImColor_SetHSV(ImColor* self,float h,float s,float v,float a)
{
    return self->SetHSV(h,s,v,a);
}
void ImColor_HSV(ImColor *pOut,ImColor* self,float h,float s,float v,float a)
{
    *pOut = self->HSV(h,s,v,a);
}
void ImDrawListSplitter_Clear(ImDrawListSplitter* self)
{
    return self->Clear();
}
void ImDrawListSplitter_ClearFreeMemory(ImDrawListSplitter* self)
{
    return self->ClearFreeMemory();
}
void ImDrawListSplitter_Split(ImDrawListSplitter* self,ImDrawList* draw_list,int count)
{
    return self->Split(draw_list,count);
}
void ImDrawListSplitter_Merge(ImDrawListSplitter* self,ImDrawList* draw_list)
{
    return self->Merge(draw_list);
}
void ImDrawListSplitter_SetCurrentChannel(ImDrawListSplitter* self,ImDrawList* draw_list,int channel_idx)
{
    return self->SetCurrentChannel(draw_list,channel_idx);
}
void ImDrawList_PushClipRect(ImDrawList* self,ImVec2 clip_rect_min,ImVec2 clip_rect_max,bool intersect_with_current_clip_rect)
{
    return self->PushClipRect(clip_rect_min,clip_rect_max,intersect_with_current_clip_rect);
}
void ImDrawList_PushClipRectFullScreen(ImDrawList* self)
{
    return self->PushClipRectFullScreen();
}
void ImDrawList_PopClipRect(ImDrawList* self)
{
    return self->PopClipRect();
}
void ImDrawList_PushTextureID(ImDrawList* self,ImTextureID texture_id)
{
    return self->PushTextureID(texture_id);
}
void ImDrawList_PopTextureID(ImDrawList* self)
{
    return self->PopTextureID();
}
void ImDrawList_GetClipRectMin(ImVec2 *pOut,ImDrawList* self)
{
    *pOut = self->GetClipRectMin();
}
void ImDrawList_GetClipRectMax(ImVec2 *pOut,ImDrawList* self)
{
    *pOut = self->GetClipRectMax();
}
void ImDrawList_AddLine(ImDrawList* self,const ImVec2 p1,const ImVec2 p2,ImU32 col,float thickness)
{
    return self->AddLine(p1,p2,col,thickness);
}
void ImDrawList_AddRect(ImDrawList* self,const ImVec2 p_min,const ImVec2 p_max,ImU32 col,float rounding,ImDrawCornerFlags rounding_corners,float thickness)
{
    return self->AddRect(p_min,p_max,col,rounding,rounding_corners,thickness);
}
void ImDrawList_AddRectFilled(ImDrawList* self,const ImVec2 p_min,const ImVec2 p_max,ImU32 col,float rounding,ImDrawCornerFlags rounding_corners)
{
    return self->AddRectFilled(p_min,p_max,col,rounding,rounding_corners);
}
void ImDrawList_AddRectFilledMultiColor(ImDrawList* self,const ImVec2 p_min,const ImVec2 p_max,ImU32 col_upr_left,ImU32 col_upr_right,ImU32 col_bot_right,ImU32 col_bot_left)
{
    return self->AddRectFilledMultiColor(p_min,p_max,col_upr_left,col_upr_right,col_bot_right,col_bot_left);
}
void ImDrawList_AddQuad(ImDrawList* self,const ImVec2 p1,const ImVec2 p2,const ImVec2 p3,const ImVec2 p4,ImU32 col,float thickness)
{
    return self->AddQuad(p1,p2,p3,p4,col,thickness);
}
void ImDrawList_AddQuadFilled(ImDrawList* self,const ImVec2 p1,const ImVec2 p2,const ImVec2 p3,const ImVec2 p4,ImU32 col)
{
    return self->AddQuadFilled(p1,p2,p3,p4,col);
}
void ImDrawList_AddTriangle(ImDrawList* self,const ImVec2 p1,const ImVec2 p2,const ImVec2 p3,ImU32 col,float thickness)
{
    return self->AddTriangle(p1,p2,p3,col,thickness);
}
void ImDrawList_AddTriangleFilled(ImDrawList* self,const ImVec2 p1,const ImVec2 p2,const ImVec2 p3,ImU32 col)
{
    return self->AddTriangleFilled(p1,p2,p3,col);
}
void ImDrawList_AddCircle(ImDrawList* self,const ImVec2 center,float radius,ImU32 col,int num_segments,float thickness)
{
    return self->AddCircle(center,radius,col,num_segments,thickness);
}
void ImDrawList_AddCircleFilled(ImDrawList* self,const ImVec2 center,float radius,ImU32 col,int num_segments)
{
    return self->AddCircleFilled(center,radius,col,num_segments);
}
void ImDrawList_AddNgon(ImDrawList* self,const ImVec2 center,float radius,ImU32 col,int num_segments,float thickness)
{
    return self->AddNgon(center,radius,col,num_segments,thickness);
}
void ImDrawList_AddNgonFilled(ImDrawList* self,const ImVec2 center,float radius,ImU32 col,int num_segments)
{
    return self->AddNgonFilled(center,radius,col,num_segments);
}
void ImDrawList_AddTextVec2(ImDrawList* self,const ImVec2 pos,ImU32 col,const char* text_begin,const char* text_end)
{
    return self->AddText(pos,col,text_begin,text_end);
}
void ImDrawList_AddTextFontPtr(ImDrawList* self,const ImFont* font,float font_size,const ImVec2 pos,ImU32 col,const char* text_begin,const char* text_end,float wrap_width,const ImVec4* cpu_fine_clip_rect)
{
    return self->AddText(font,font_size,pos,col,text_begin,text_end,wrap_width,cpu_fine_clip_rect);
}
void ImDrawList_AddPolyline(ImDrawList* self,const ImVec2* points,int num_points,ImU32 col,bool closed,float thickness)
{
    return self->AddPolyline(points,num_points,col,closed,thickness);
}
void ImDrawList_AddConvexPolyFilled(ImDrawList* self,const ImVec2* points,int num_points,ImU32 col)
{
    return self->AddConvexPolyFilled(points,num_points,col);
}
void ImDrawList_AddBezierCurve(ImDrawList* self,const ImVec2 p1,const ImVec2 p2,const ImVec2 p3,const ImVec2 p4,ImU32 col,float thickness,int num_segments)
{
    return self->AddBezierCurve(p1,p2,p3,p4,col,thickness,num_segments);
}
void ImDrawList_AddImage(ImDrawList* self,ImTextureID user_texture_id,const ImVec2 p_min,const ImVec2 p_max,const ImVec2 uv_min,const ImVec2 uv_max,ImU32 col)
{
    return self->AddImage(user_texture_id,p_min,p_max,uv_min,uv_max,col);
}
void ImDrawList_AddImageQuad(ImDrawList* self,ImTextureID user_texture_id,const ImVec2 p1,const ImVec2 p2,const ImVec2 p3,const ImVec2 p4,const ImVec2 uv1,const ImVec2 uv2,const ImVec2 uv3,const ImVec2 uv4,ImU32 col)
{
    return self->AddImageQuad(user_texture_id,p1,p2,p3,p4,uv1,uv2,uv3,uv4,col);
}
void ImDrawList_AddImageRounded(ImDrawList* self,ImTextureID user_texture_id,const ImVec2 p_min,const ImVec2 p_max,const ImVec2 uv_min,const ImVec2 uv_max,ImU32 col,float rounding,ImDrawCornerFlags rounding_corners)
{
    return self->AddImageRounded(user_texture_id,p_min,p_max,uv_min,uv_max,col,rounding,rounding_corners);
}
void ImDrawList_PathClear(ImDrawList* self)
{
    return self->PathClear();
}
void ImDrawList_PathLineTo(ImDrawList* self,const ImVec2 pos)
{
    return self->PathLineTo(pos);
}
void ImDrawList_PathLineToMergeDuplicate(ImDrawList* self,const ImVec2 pos)
{
    return self->PathLineToMergeDuplicate(pos);
}
void ImDrawList_PathFillConvex(ImDrawList* self,ImU32 col)
{
    return self->PathFillConvex(col);
}
void ImDrawList_PathStroke(ImDrawList* self,ImU32 col,bool closed,float thickness)
{
    return self->PathStroke(col,closed,thickness);
}
void ImDrawList_PathArcTo(ImDrawList* self,const ImVec2 center,float radius,float a_min,float a_max,int num_segments)
{
    return self->PathArcTo(center,radius,a_min,a_max,num_segments);
}
void ImDrawList_PathArcToFast(ImDrawList* self,const ImVec2 center,float radius,int a_min_of_12,int a_max_of_12)
{
    return self->PathArcToFast(center,radius,a_min_of_12,a_max_of_12);
}
void ImDrawList_PathBezierCurveTo(ImDrawList* self,const ImVec2 p2,const ImVec2 p3,const ImVec2 p4,int num_segments)
{
    return self->PathBezierCurveTo(p2,p3,p4,num_segments);
}
void ImDrawList_PathRect(ImDrawList* self,const ImVec2 rect_min,const ImVec2 rect_max,float rounding,ImDrawCornerFlags rounding_corners)
{
    return self->PathRect(rect_min,rect_max,rounding,rounding_corners);
}
void ImDrawList_AddCallback(ImDrawList* self,ImDrawCallback callback,void* callback_data)
{
    return self->AddCallback(callback,callback_data);
}
void ImDrawList_AddDrawCmd(ImDrawList* self)
{
    return self->AddDrawCmd();
}
ImDrawList* ImDrawList_CloneOutput(ImDrawList* self)
{
    return self->CloneOutput();
}
void ImDrawList_ChannelsSplit(ImDrawList* self,int count)
{
    return self->ChannelsSplit(count);
}
void ImDrawList_ChannelsMerge(ImDrawList* self)
{
    return self->ChannelsMerge();
}
void ImDrawList_ChannelsSetCurrent(ImDrawList* self,int n)
{
    return self->ChannelsSetCurrent(n);
}
void ImDrawList_Clear(ImDrawList* self)
{
    return self->Clear();
}
void ImDrawList_ClearFreeMemory(ImDrawList* self)
{
    return self->ClearFreeMemory();
}
void ImDrawList_PrimReserve(ImDrawList* self,int idx_count,int vtx_count)
{
    return self->PrimReserve(idx_count,vtx_count);
}
void ImDrawList_PrimUnreserve(ImDrawList* self,int idx_count,int vtx_count)
{
    return self->PrimUnreserve(idx_count,vtx_count);
}
void ImDrawList_PrimRect(ImDrawList* self,const ImVec2 a,const ImVec2 b,ImU32 col)
{
    return self->PrimRect(a,b,col);
}
void ImDrawList_PrimRectUV(ImDrawList* self,const ImVec2 a,const ImVec2 b,const ImVec2 uv_a,const ImVec2 uv_b,ImU32 col)
{
    return self->PrimRectUV(a,b,uv_a,uv_b,col);
}
void ImDrawList_PrimQuadUV(ImDrawList* self,const ImVec2 a,const ImVec2 b,const ImVec2 c,const ImVec2 d,const ImVec2 uv_a,const ImVec2 uv_b,const ImVec2 uv_c,const ImVec2 uv_d,ImU32 col)
{
    return self->PrimQuadUV(a,b,c,d,uv_a,uv_b,uv_c,uv_d,col);
}
void ImDrawList_PrimWriteVtx(ImDrawList* self,const ImVec2 pos,const ImVec2 uv,ImU32 col)
{
    return self->PrimWriteVtx(pos,uv,col);
}
void ImDrawList_PrimWriteIdx(ImDrawList* self,ImDrawIdx idx)
{
    return self->PrimWriteIdx(idx);
}
void ImDrawList_PrimVtx(ImDrawList* self,const ImVec2 pos,const ImVec2 uv,ImU32 col)
{
    return self->PrimVtx(pos,uv,col);
}
void ImDrawList_UpdateClipRect(ImDrawList* self)
{
    return self->UpdateClipRect();
}
void ImDrawList_UpdateTextureID(ImDrawList* self)
{
    return self->UpdateTextureID();
}
void ImDrawData_Clear(ImDrawData* self)
{
    return self->Clear();
}
void ImDrawData_DeIndexAllBuffers(ImDrawData* self)
{
    return self->DeIndexAllBuffers();
}
void ImDrawData_ScaleClipRects(ImDrawData* self,const ImVec2 fb_scale)
{
    return self->ScaleClipRects(fb_scale);
}
void ImFontGlyphRangesBuilder_Clear(ImFontGlyphRangesBuilder* self)
{
    return self->Clear();
}
bool ImFontGlyphRangesBuilder_GetBit(ImFontGlyphRangesBuilder* self,int n)
{
    return self->GetBit(n);
}
void ImFontGlyphRangesBuilder_SetBit(ImFontGlyphRangesBuilder* self,int n)
{
    return self->SetBit(n);
}
void ImFontGlyphRangesBuilder_AddChar(ImFontGlyphRangesBuilder* self,ImWchar c)
{
    return self->AddChar(c);
}
void ImFontGlyphRangesBuilder_AddText(ImFontGlyphRangesBuilder* self,const char* text,const char* text_end)
{
    return self->AddText(text,text_end);
}
void ImFontGlyphRangesBuilder_AddRanges(ImFontGlyphRangesBuilder* self,const ImWchar* ranges)
{
    return self->AddRanges(ranges);
}
bool ImFontAtlasCustomRect_IsPacked(ImFontAtlasCustomRect* self)
{
    return self->IsPacked();
}
ImFont* ImFontAtlas_AddFont(ImFontAtlas* self,const ImFontConfig* font_cfg)
{
    return self->AddFont(font_cfg);
}
ImFont* ImFontAtlas_AddFontDefault(ImFontAtlas* self,const ImFontConfig* font_cfg)
{
    return self->AddFontDefault(font_cfg);
}
ImFont* ImFontAtlas_AddFontFromFileTTF(ImFontAtlas* self,const char* filename,float size_pixels,const ImFontConfig* font_cfg,const ImWchar* glyph_ranges)
{
    return self->AddFontFromFileTTF(filename,size_pixels,font_cfg,glyph_ranges);
}
ImFont* ImFontAtlas_AddFontFromMemoryTTF(ImFontAtlas* self,void* font_data,int font_size,float size_pixels,const ImFontConfig* font_cfg,const ImWchar* glyph_ranges)
{
    return self->AddFontFromMemoryTTF(font_data,font_size,size_pixels,font_cfg,glyph_ranges);
}
ImFont* ImFontAtlas_AddFontFromMemoryCompressedTTF(ImFontAtlas* self,const void* compressed_font_data,int compressed_font_size,float size_pixels,const ImFontConfig* font_cfg,const ImWchar* glyph_ranges)
{
    return self->AddFontFromMemoryCompressedTTF(compressed_font_data,compressed_font_size,size_pixels,font_cfg,glyph_ranges);
}
ImFont* ImFontAtlas_AddFontFromMemoryCompressedBase85TTF(ImFontAtlas* self,const char* compressed_font_data_base85,float size_pixels,const ImFontConfig* font_cfg,const ImWchar* glyph_ranges)
{
    return self->AddFontFromMemoryCompressedBase85TTF(compressed_font_data_base85,size_pixels,font_cfg,glyph_ranges);
}
void ImFontAtlas_ClearInputData(ImFontAtlas* self)
{
    return self->ClearInputData();
}
void ImFontAtlas_ClearTexData(ImFontAtlas* self)
{
    return self->ClearTexData();
}
void ImFontAtlas_ClearFonts(ImFontAtlas* self)
{
    return self->ClearFonts();
}
void ImFontAtlas_Clear(ImFontAtlas* self)
{
    return self->Clear();
}
bool ImFontAtlas_Build(ImFontAtlas* self)
{
    return self->Build();
}
void ImFontAtlas_GetTexDataAsAlpha8(ImFontAtlas* self,unsigned char** out_pixels,int* out_width,int* out_height,int* out_bytes_per_pixel)
{
    return self->GetTexDataAsAlpha8(out_pixels,out_width,out_height,out_bytes_per_pixel);
}
void ImFontAtlas_GetTexDataAsRGBA32(ImFontAtlas* self,unsigned char** out_pixels,int* out_width,int* out_height,int* out_bytes_per_pixel)
{
    return self->GetTexDataAsRGBA32(out_pixels,out_width,out_height,out_bytes_per_pixel);
}
bool ImFontAtlas_IsBuilt(ImFontAtlas* self)
{
    return self->IsBuilt();
}
void ImFontAtlas_SetTexID(ImFontAtlas* self,ImTextureID id)
{
    return self->SetTexID(id);
}
const ImWchar* ImFontAtlas_GetGlyphRangesDefault(ImFontAtlas* self)
{
    return self->GetGlyphRangesDefault();
}
const ImWchar* ImFontAtlas_GetGlyphRangesKorean(ImFontAtlas* self)
{
    return self->GetGlyphRangesKorean();
}
const ImWchar* ImFontAtlas_GetGlyphRangesJapanese(ImFontAtlas* self)
{
    return self->GetGlyphRangesJapanese();
}
const ImWchar* ImFontAtlas_GetGlyphRangesChineseFull(ImFontAtlas* self)
{
    return self->GetGlyphRangesChineseFull();
}
const ImWchar* ImFontAtlas_GetGlyphRangesChineseSimplifiedCommon(ImFontAtlas* self)
{
    return self->GetGlyphRangesChineseSimplifiedCommon();
}
const ImWchar* ImFontAtlas_GetGlyphRangesCyrillic(ImFontAtlas* self)
{
    return self->GetGlyphRangesCyrillic();
}
const ImWchar* ImFontAtlas_GetGlyphRangesThai(ImFontAtlas* self)
{
    return self->GetGlyphRangesThai();
}
const ImWchar* ImFontAtlas_GetGlyphRangesVietnamese(ImFontAtlas* self)
{
    return self->GetGlyphRangesVietnamese();
}
int ImFontAtlas_AddCustomRectRegular(ImFontAtlas* self,unsigned int id,int width,int height)
{
    return self->AddCustomRectRegular(id,width,height);
}
int ImFontAtlas_AddCustomRectFontGlyph(ImFontAtlas* self,ImFont* font,ImWchar id,int width,int height,float advance_x,const ImVec2 offset)
{
    return self->AddCustomRectFontGlyph(font,id,width,height,advance_x,offset);
}
const ImFontAtlasCustomRect* ImFontAtlas_GetCustomRectByIndex(ImFontAtlas* self,int index)
{
    return self->GetCustomRectByIndex(index);
}
void ImFontAtlas_CalcCustomRectUV(ImFontAtlas* self,const ImFontAtlasCustomRect* rect,ImVec2* out_uv_min,ImVec2* out_uv_max)
{
    return self->CalcCustomRectUV(rect,out_uv_min,out_uv_max);
}
bool ImFontAtlas_GetMouseCursorTexData(ImFontAtlas* self,ImGuiMouseCursor cursor,ImVec2* out_offset,ImVec2* out_size,ImVec2 out_uv_border[2],ImVec2 out_uv_fill[2])
{
    return self->GetMouseCursorTexData(cursor,out_offset,out_size,out_uv_border,out_uv_fill);
}
const ImFontGlyph* ImFont_FindGlyph(ImFont* self,ImWchar c)
{
    return self->FindGlyph(c);
}
const ImFontGlyph* ImFont_FindGlyphNoFallback(ImFont* self,ImWchar c)
{
    return self->FindGlyphNoFallback(c);
}
float ImFont_GetCharAdvance(ImFont* self,ImWchar c)
{
    return self->GetCharAdvance(c);
}
bool ImFont_IsLoaded(ImFont* self)
{
    return self->IsLoaded();
}
const char* ImFont_GetDebugName(ImFont* self)
{
    return self->GetDebugName();
}
void ImFont_CalcTextSizeA(ImVec2 *pOut,ImFont* self,float size,float max_width,float wrap_width,const char* text_begin,const char* text_end,const char** remaining)
{
    *pOut = self->CalcTextSizeA(size,max_width,wrap_width,text_begin,text_end,remaining);
}
const char* ImFont_CalcWordWrapPositionA(ImFont* self,float scale,const char* text,const char* text_end,float wrap_width)
{
    return self->CalcWordWrapPositionA(scale,text,text_end,wrap_width);
}
void ImFont_RenderChar(ImFont* self,ImDrawList* draw_list,float size,ImVec2 pos,ImU32 col,ImWchar c)
{
    return self->RenderChar(draw_list,size,pos,col,c);
}
void ImFont_RenderText(ImFont* self,ImDrawList* draw_list,float size,ImVec2 pos,ImU32 col,const ImVec4 clip_rect,const char* text_begin,const char* text_end,float wrap_width,bool cpu_fine_clip)
{
    return self->RenderText(draw_list,size,pos,col,clip_rect,text_begin,text_end,wrap_width,cpu_fine_clip);
}
void ImFont_BuildLookupTable(ImFont* self)
{
    return self->BuildLookupTable();
}
void ImFont_ClearOutputData(ImFont* self)
{
    return self->ClearOutputData();
}
void ImFont_GrowIndex(ImFont* self,int new_size)
{
    return self->GrowIndex(new_size);
}
void ImFont_AddGlyph(ImFont* self,ImWchar c,float x0,float y0,float x1,float y1,float u0,float v0,float u1,float v1,float advance_x)
{
    return self->AddGlyph(c,x0,y0,x1,y1,u0,v0,u1,v1,advance_x);
}
void ImFont_AddRemapChar(ImFont* self,ImWchar dst,ImWchar src,bool overwrite_dst)
{
    return self->AddRemapChar(dst,src,overwrite_dst);
}
void ImFont_SetFallbackChar(ImFont* self,ImWchar c)
{
    return self->SetFallbackChar(c);
}



/////////////////////////////manual written functions
void igLogText(const char *fmt, ...)
{
    char buffer[256];
    VaList args = VA_START(fmt);
    vsnprintf(buffer, 256, fmt, args);

    ImGui::LogText("%s", buffer);
}
void ImGuiTextBuffer_appendf(struct ImGuiTextBuffer *buffer, const char *fmt, ...)
{
    VaList args = VA_START(fmt);
    buffer->appendfv(fmt, args);
}

float igGET_FLT_MAX()
{
    return FLT_MAX;
}
void igColorConvertRGBtoHSV(float r,float g,float b,float *out_h,float *out_s,float *out_v)
{
    ImGui::ColorConvertRGBtoHSV(r,g,b,*out_h,*out_s,*out_v);
}
void igColorConvertHSVtoRGB(float h,float s,float v,float *out_r,float *out_g,float *out_b)
{
    ImGui::ColorConvertHSVtoRGB(h,s,v,*out_r,*out_g,*out_b);
}

#ifdef IMGUI_HAS_DOCK

// NOTE: Some function pointers in the ImGuiPlatformIO structure are not C-compatible because of their
// use of a complex return type. To work around this, we store a custom CimguiStorage object inside
// ImGuiIO::BackendLanguageUserData, which contains C-compatible function pointer variants for these
// functions. When a user function pointer is provided, we hook up the underlying ImGuiPlatformIO
// function pointer to a thunk which accesses the user function pointer through CimguiStorage.

struct CimguiStorage
{
    void(*Platform_GetWindowPos)(ImGuiViewport* vp, ImVec2* out_pos);
    void(*Platform_GetWindowSize)(ImGuiViewport* vp, ImVec2* out_pos);
};

// Gets a reference to the CimguiStorage object stored in the current ImGui context's BackendLanguageUserData.
CimguiStorage& GetCimguiStorage()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.BackendLanguageUserData == NULL)
    {
        io.BackendLanguageUserData = new CimguiStorage();
    }

    return *(CimguiStorage*)io.BackendLanguageUserData;
}

// Thunk satisfying the signature of ImGuiPlatformIO::Platform_GetWindowPos.
ImVec2 Platform_GetWindowPos_hook(ImGuiViewport* vp)
{
    ImVec2 pos;
    GetCimguiStorage().Platform_GetWindowPos(vp, &pos);
    return pos;
};

// Fully C-compatible function pointer setter for ImGuiPlatformIO::Platform_GetWindowPos.
void ImGuiPlatformIO_Set_Platform_GetWindowPos(ImGuiPlatformIO* platform_io, void(*user_callback)(ImGuiViewport* vp, ImVec2* out_pos))
{
    CimguiStorage& storage = GetCimguiStorage();
    storage.Platform_GetWindowPos = user_callback;
    platform_io->Platform_GetWindowPos = &Platform_GetWindowPos_hook;
}

// Thunk satisfying the signature of ImGuiPlatformIO::Platform_GetWindowSize.
ImVec2 Platform_GetWindowSize_hook(ImGuiViewport* vp)
{
    ImVec2 size;
    GetCimguiStorage().Platform_GetWindowSize(vp, &size);
    return size;
};

// Fully C-compatible function pointer setter for ImGuiPlatformIO::Platform_GetWindowSize.
void ImGuiPlatformIO_Set_Platform_GetWindowSize(ImGuiPlatformIO* platform_io, void(*user_callback)(ImGuiViewport* vp, ImVec2* out_size))
{
    CimguiStorage& storage = GetCimguiStorage();
    storage.Platform_GetWindowSize = user_callback;
    platform_io->Platform_GetWindowSize = &Platform_GetWindowSize_hook;
}

#endif

} // extern "C"
