#ifndef CIMGUI_INCLUDED
#define CIMGUI_INCLUDED

#include "common/macro.h"

PIM_C_BEGIN

#include "common/valist.h"

#ifndef CIMGUI_VERSION
    #define CIMGUI_VERSION      "1.75"
    #define CIMGUI_VERSION_NUM  17500
#endif

typedef signed char ImS8;
typedef unsigned char ImU8;
typedef signed short ImS16;
typedef unsigned short ImU16;
typedef signed int ImS32;
typedef unsigned int ImU32;
typedef signed long long ImS64;
typedef unsigned long long ImU64;

typedef struct ImVec2
{
    float x, y;
} ImVec2;

typedef struct ImVec4
{
    float x, y, z, w;
} ImVec4;

typedef struct ImGuiStoragePair ImGuiStoragePair;
typedef struct ImGuiTextRange ImGuiTextRange;
typedef struct ImFontAtlasCustomRect ImFontAtlasCustomRect;
typedef struct ImVec4 ImVec4;
typedef struct ImVec2 ImVec2;
typedef struct ImGuiTextFilter ImGuiTextFilter;
typedef struct ImGuiTextBuffer ImGuiTextBuffer;
typedef struct ImGuiStyle ImGuiStyle;
typedef struct ImGuiStorage ImGuiStorage;
typedef struct ImGuiSizeCallbackData ImGuiSizeCallbackData;
typedef struct ImGuiPayload ImGuiPayload;
typedef struct ImGuiOnceUponAFrame ImGuiOnceUponAFrame;
typedef struct ImGuiListClipper ImGuiListClipper;
typedef struct ImGuiInputTextCallbackData ImGuiInputTextCallbackData;
typedef struct ImGuiIO ImGuiIO;
typedef struct ImGuiContext ImGuiContext;
typedef struct ImColor ImColor;
typedef struct ImFontGlyphRangesBuilder ImFontGlyphRangesBuilder;
typedef struct ImFontGlyph ImFontGlyph;
typedef struct ImFontConfig ImFontConfig;
typedef struct ImFontAtlas ImFontAtlas;
typedef struct ImFont ImFont;
typedef struct ImDrawVert ImDrawVert;
typedef struct ImDrawListSplitter ImDrawListSplitter;
typedef struct ImDrawListSharedData ImDrawListSharedData;
typedef struct ImDrawList ImDrawList;
typedef struct ImDrawData ImDrawData;
typedef struct ImDrawCmd ImDrawCmd;
typedef struct ImDrawChannel ImDrawChannel;

typedef void* ImTextureID;
typedef unsigned int ImGuiID;
typedef unsigned short ImWchar;
typedef int ImGuiCol;
typedef int ImGuiCond;
typedef int ImGuiDataType;
typedef int ImGuiDir;
typedef int ImGuiKey;
typedef int ImGuiNavInput;
typedef int ImGuiMouseButton;
typedef int ImGuiMouseCursor;
typedef int ImGuiStyleVar;
typedef int ImDrawCornerFlags;
typedef int ImDrawListFlags;
typedef int ImFontAtlasFlags;
typedef int ImGuiBackendFlags;
typedef int ImGuiColorEditFlags;
typedef int ImGuiConfigFlags;
typedef int ImGuiComboFlags;
typedef int ImGuiDragDropFlags;
typedef int ImGuiFocusedFlags;
typedef int ImGuiHoveredFlags;
typedef int ImGuiInputTextFlags;
typedef int ImGuiSelectableFlags;
typedef int ImGuiTabBarFlags;
typedef int ImGuiTabItemFlags;
typedef int ImGuiTreeNodeFlags;
typedef int ImGuiWindowFlags;
typedef int (*ImGuiInputTextCallback)(struct ImGuiInputTextCallbackData *data);
typedef void (*ImGuiSizeCallback)(struct ImGuiSizeCallbackData* data);
typedef void (*ImDrawCallback)(const struct ImDrawList* parent_list, const struct ImDrawCmd* cmd);
typedef unsigned short ImDrawIdx;

typedef struct ImVector
{int Size;int Capacity;void* Data;} ImVector;

typedef struct ImVector_float
{int Size;int Capacity;float* Data;} ImVector_float;

typedef struct ImVector_ImWchar
{int Size;int Capacity;ImWchar* Data;} ImVector_ImWchar;

typedef struct ImVector_ImDrawVert
{int Size;int Capacity; struct ImDrawVert* Data;} ImVector_ImDrawVert;

typedef struct ImVector_ImFontGlyph
{int Size;int Capacity; struct ImFontGlyph* Data;} ImVector_ImFontGlyph;

typedef struct ImVector_ImGuiTextRange
{int Size;int Capacity; struct ImGuiTextRange* Data;} ImVector_ImGuiTextRange;

typedef struct ImVector_ImGuiStoragePair
{int Size;int Capacity; struct ImGuiStoragePair* Data;} ImVector_ImGuiStoragePair;

typedef struct ImVector_ImDrawChannel
{int Size;int Capacity; struct ImDrawChannel* Data;} ImVector_ImDrawChannel;

typedef struct ImVector_char
{int Size;int Capacity;char* Data;} ImVector_char;

typedef struct ImVector_ImU32
{int Size;int Capacity;ImU32* Data;} ImVector_ImU32;

typedef struct ImVector_ImFontAtlasCustomRect
{int Size;int Capacity; struct ImFontAtlasCustomRect* Data;} ImVector_ImFontAtlasCustomRect;

typedef struct ImVector_ImTextureID
{int Size;int Capacity;ImTextureID* Data;} ImVector_ImTextureID;

typedef struct ImVector_ImFontConfig
{int Size;int Capacity; struct ImFontConfig* Data;} ImVector_ImFontConfig;

typedef struct ImVector_ImFontPtr
{int Size;int Capacity; struct ImFont** Data;} ImVector_ImFontPtr;

typedef struct ImVector_ImDrawCmd
{int Size;int Capacity; struct ImDrawCmd* Data;} ImVector_ImDrawCmd;

typedef struct ImVector_ImVec4
{int Size;int Capacity;ImVec4* Data;} ImVector_ImVec4;

typedef struct ImVector_ImDrawIdx
{int Size;int Capacity;ImDrawIdx* Data;} ImVector_ImDrawIdx;

typedef struct ImVector_ImVec2
{int Size;int Capacity;ImVec2* Data;} ImVector_ImVec2;

typedef enum {
    ImGuiWindowFlags_None = 0,
    ImGuiWindowFlags_NoTitleBar = 1 << 0,
    ImGuiWindowFlags_NoResize = 1 << 1,
    ImGuiWindowFlags_NoMove = 1 << 2,
    ImGuiWindowFlags_NoScrollbar = 1 << 3,
    ImGuiWindowFlags_NoScrollWithMouse = 1 << 4,
    ImGuiWindowFlags_NoCollapse = 1 << 5,
    ImGuiWindowFlags_AlwaysAutoResize = 1 << 6,
    ImGuiWindowFlags_NoBackground = 1 << 7,
    ImGuiWindowFlags_NoSavedSettings = 1 << 8,
    ImGuiWindowFlags_NoMouseInputs = 1 << 9,
    ImGuiWindowFlags_MenuBar = 1 << 10,
    ImGuiWindowFlags_HorizontalScrollbar = 1 << 11,
    ImGuiWindowFlags_NoFocusOnAppearing = 1 << 12,
    ImGuiWindowFlags_NoBringToFrontOnFocus = 1 << 13,
    ImGuiWindowFlags_AlwaysVerticalScrollbar= 1 << 14,
    ImGuiWindowFlags_AlwaysHorizontalScrollbar=1<< 15,
    ImGuiWindowFlags_AlwaysUseWindowPadding = 1 << 16,
    ImGuiWindowFlags_NoNavInputs = 1 << 18,
    ImGuiWindowFlags_NoNavFocus = 1 << 19,
    ImGuiWindowFlags_UnsavedDocument = 1 << 20,
    ImGuiWindowFlags_NoNav = ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus,
    ImGuiWindowFlags_NoDecoration = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse,
    ImGuiWindowFlags_NoInputs = ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus,
    ImGuiWindowFlags_NavFlattened = 1 << 23,
    ImGuiWindowFlags_ChildWindow = 1 << 24,
    ImGuiWindowFlags_Tooltip = 1 << 25,
    ImGuiWindowFlags_Popup = 1 << 26,
    ImGuiWindowFlags_Modal = 1 << 27,
    ImGuiWindowFlags_ChildMenu = 1 << 28
}ImGuiWindowFlags_;
typedef enum {
    ImGuiInputTextFlags_None = 0,
    ImGuiInputTextFlags_CharsDecimal = 1 << 0,
    ImGuiInputTextFlags_CharsHexadecimal = 1 << 1,
    ImGuiInputTextFlags_CharsUppercase = 1 << 2,
    ImGuiInputTextFlags_CharsNoBlank = 1 << 3,
    ImGuiInputTextFlags_AutoSelectAll = 1 << 4,
    ImGuiInputTextFlags_EnterReturnsTrue = 1 << 5,
    ImGuiInputTextFlags_CallbackCompletion = 1 << 6,
    ImGuiInputTextFlags_CallbackHistory = 1 << 7,
    ImGuiInputTextFlags_CallbackAlways = 1 << 8,
    ImGuiInputTextFlags_CallbackCharFilter = 1 << 9,
    ImGuiInputTextFlags_AllowTabInput = 1 << 10,
    ImGuiInputTextFlags_CtrlEnterForNewLine = 1 << 11,
    ImGuiInputTextFlags_NoHorizontalScroll = 1 << 12,
    ImGuiInputTextFlags_AlwaysInsertMode = 1 << 13,
    ImGuiInputTextFlags_ReadOnly = 1 << 14,
    ImGuiInputTextFlags_Password = 1 << 15,
    ImGuiInputTextFlags_NoUndoRedo = 1 << 16,
    ImGuiInputTextFlags_CharsScientific = 1 << 17,
    ImGuiInputTextFlags_CallbackResize = 1 << 18,
    ImGuiInputTextFlags_Multiline = 1 << 20,
    ImGuiInputTextFlags_NoMarkEdited = 1 << 21
}ImGuiInputTextFlags_;
typedef enum {
    ImGuiTreeNodeFlags_None = 0,
    ImGuiTreeNodeFlags_Selected = 1 << 0,
    ImGuiTreeNodeFlags_Framed = 1 << 1,
    ImGuiTreeNodeFlags_AllowItemOverlap = 1 << 2,
    ImGuiTreeNodeFlags_NoTreePushOnOpen = 1 << 3,
    ImGuiTreeNodeFlags_NoAutoOpenOnLog = 1 << 4,
    ImGuiTreeNodeFlags_DefaultOpen = 1 << 5,
    ImGuiTreeNodeFlags_OpenOnDoubleClick = 1 << 6,
    ImGuiTreeNodeFlags_OpenOnArrow = 1 << 7,
    ImGuiTreeNodeFlags_Leaf = 1 << 8,
    ImGuiTreeNodeFlags_Bullet = 1 << 9,
    ImGuiTreeNodeFlags_FramePadding = 1 << 10,
    ImGuiTreeNodeFlags_SpanAvailWidth = 1 << 11,
    ImGuiTreeNodeFlags_SpanFullWidth = 1 << 12,
    ImGuiTreeNodeFlags_NavLeftJumpsBackHere = 1 << 13,
    ImGuiTreeNodeFlags_CollapsingHeader = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_NoAutoOpenOnLog
}ImGuiTreeNodeFlags_;
typedef enum {
    ImGuiSelectableFlags_None = 0,
    ImGuiSelectableFlags_DontClosePopups = 1 << 0,
    ImGuiSelectableFlags_SpanAllColumns = 1 << 1,
    ImGuiSelectableFlags_AllowDoubleClick = 1 << 2,
    ImGuiSelectableFlags_Disabled = 1 << 3,
    ImGuiSelectableFlags_AllowItemOverlap = 1 << 4
}ImGuiSelectableFlags_;
typedef enum {
    ImGuiComboFlags_None = 0,
    ImGuiComboFlags_PopupAlignLeft = 1 << 0,
    ImGuiComboFlags_HeightSmall = 1 << 1,
    ImGuiComboFlags_HeightRegular = 1 << 2,
    ImGuiComboFlags_HeightLarge = 1 << 3,
    ImGuiComboFlags_HeightLargest = 1 << 4,
    ImGuiComboFlags_NoArrowButton = 1 << 5,
    ImGuiComboFlags_NoPreview = 1 << 6,
    ImGuiComboFlags_HeightMask_ = ImGuiComboFlags_HeightSmall | ImGuiComboFlags_HeightRegular | ImGuiComboFlags_HeightLarge | ImGuiComboFlags_HeightLargest
}ImGuiComboFlags_;
typedef enum {
    ImGuiTabBarFlags_None = 0,
    ImGuiTabBarFlags_Reorderable = 1 << 0,
    ImGuiTabBarFlags_AutoSelectNewTabs = 1 << 1,
    ImGuiTabBarFlags_TabListPopupButton = 1 << 2,
    ImGuiTabBarFlags_NoCloseWithMiddleMouseButton = 1 << 3,
    ImGuiTabBarFlags_NoTabListScrollingButtons = 1 << 4,
    ImGuiTabBarFlags_NoTooltip = 1 << 5,
    ImGuiTabBarFlags_FittingPolicyResizeDown = 1 << 6,
    ImGuiTabBarFlags_FittingPolicyScroll = 1 << 7,
    ImGuiTabBarFlags_FittingPolicyMask_ = ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_FittingPolicyScroll,
    ImGuiTabBarFlags_FittingPolicyDefault_ = ImGuiTabBarFlags_FittingPolicyResizeDown
}ImGuiTabBarFlags_;
typedef enum {
    ImGuiTabItemFlags_None = 0,
    ImGuiTabItemFlags_UnsavedDocument = 1 << 0,
    ImGuiTabItemFlags_SetSelected = 1 << 1,
    ImGuiTabItemFlags_NoCloseWithMiddleMouseButton = 1 << 2,
    ImGuiTabItemFlags_NoPushId = 1 << 3
}ImGuiTabItemFlags_;
typedef enum {
    ImGuiFocusedFlags_None = 0,
    ImGuiFocusedFlags_ChildWindows = 1 << 0,
    ImGuiFocusedFlags_RootWindow = 1 << 1,
    ImGuiFocusedFlags_AnyWindow = 1 << 2,
    ImGuiFocusedFlags_RootAndChildWindows = ImGuiFocusedFlags_RootWindow | ImGuiFocusedFlags_ChildWindows
}ImGuiFocusedFlags_;
typedef enum {
    ImGuiHoveredFlags_None = 0,
    ImGuiHoveredFlags_ChildWindows = 1 << 0,
    ImGuiHoveredFlags_RootWindow = 1 << 1,
    ImGuiHoveredFlags_AnyWindow = 1 << 2,
    ImGuiHoveredFlags_AllowWhenBlockedByPopup = 1 << 3,
    ImGuiHoveredFlags_AllowWhenBlockedByActiveItem = 1 << 5,
    ImGuiHoveredFlags_AllowWhenOverlapped = 1 << 6,
    ImGuiHoveredFlags_AllowWhenDisabled = 1 << 7,
    ImGuiHoveredFlags_RectOnly = ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenOverlapped,
    ImGuiHoveredFlags_RootAndChildWindows = ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows
}ImGuiHoveredFlags_;
typedef enum {
    ImGuiDragDropFlags_None = 0,
    ImGuiDragDropFlags_SourceNoPreviewTooltip = 1 << 0,
    ImGuiDragDropFlags_SourceNoDisableHover = 1 << 1,
    ImGuiDragDropFlags_SourceNoHoldToOpenOthers = 1 << 2,
    ImGuiDragDropFlags_SourceAllowNullID = 1 << 3,
    ImGuiDragDropFlags_SourceExtern = 1 << 4,
    ImGuiDragDropFlags_SourceAutoExpirePayload = 1 << 5,
    ImGuiDragDropFlags_AcceptBeforeDelivery = 1 << 10,
    ImGuiDragDropFlags_AcceptNoDrawDefaultRect = 1 << 11,
    ImGuiDragDropFlags_AcceptNoPreviewTooltip = 1 << 12,
    ImGuiDragDropFlags_AcceptPeekOnly = ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect
}ImGuiDragDropFlags_;
typedef enum {
    ImGuiDataType_S8,
    ImGuiDataType_U8,
    ImGuiDataType_S16,
    ImGuiDataType_U16,
    ImGuiDataType_S32,
    ImGuiDataType_U32,
    ImGuiDataType_S64,
    ImGuiDataType_U64,
    ImGuiDataType_Float,
    ImGuiDataType_Double,
    ImGuiDataType_COUNT
}ImGuiDataType_;
typedef enum {
    ImGuiDir_None = -1,
    ImGuiDir_Left = 0,
    ImGuiDir_Right = 1,
    ImGuiDir_Up = 2,
    ImGuiDir_Down = 3,
    ImGuiDir_COUNT
}ImGuiDir_;
typedef enum {
    ImGuiKey_Tab,
    ImGuiKey_LeftArrow,
    ImGuiKey_RightArrow,
    ImGuiKey_UpArrow,
    ImGuiKey_DownArrow,
    ImGuiKey_PageUp,
    ImGuiKey_PageDown,
    ImGuiKey_Home,
    ImGuiKey_End,
    ImGuiKey_Insert,
    ImGuiKey_Delete,
    ImGuiKey_Backspace,
    ImGuiKey_Space,
    ImGuiKey_Enter,
    ImGuiKey_Escape,
    ImGuiKey_KeyPadEnter,
    ImGuiKey_A,
    ImGuiKey_C,
    ImGuiKey_V,
    ImGuiKey_X,
    ImGuiKey_Y,
    ImGuiKey_Z,
    ImGuiKey_COUNT
}ImGuiKey_;
typedef enum {
    ImGuiNavInput_Activate,
    ImGuiNavInput_Cancel,
    ImGuiNavInput_Input,
    ImGuiNavInput_Menu,
    ImGuiNavInput_DpadLeft,
    ImGuiNavInput_DpadRight,
    ImGuiNavInput_DpadUp,
    ImGuiNavInput_DpadDown,
    ImGuiNavInput_LStickLeft,
    ImGuiNavInput_LStickRight,
    ImGuiNavInput_LStickUp,
    ImGuiNavInput_LStickDown,
    ImGuiNavInput_FocusPrev,
    ImGuiNavInput_FocusNext,
    ImGuiNavInput_TweakSlow,
    ImGuiNavInput_TweakFast,
    ImGuiNavInput_KeyMenu_,
    ImGuiNavInput_KeyLeft_,
    ImGuiNavInput_KeyRight_,
    ImGuiNavInput_KeyUp_,
    ImGuiNavInput_KeyDown_,
    ImGuiNavInput_COUNT,
    ImGuiNavInput_InternalStart_ = ImGuiNavInput_KeyMenu_
}ImGuiNavInput_;
typedef enum {
    ImGuiConfigFlags_None = 0,
    ImGuiConfigFlags_NavEnableKeyboard = 1 << 0,
    ImGuiConfigFlags_NavEnableGamepad = 1 << 1,
    ImGuiConfigFlags_NavEnableSetMousePos = 1 << 2,
    ImGuiConfigFlags_NavNoCaptureKeyboard = 1 << 3,
    ImGuiConfigFlags_NoMouse = 1 << 4,
    ImGuiConfigFlags_NoMouseCursorChange = 1 << 5,
    ImGuiConfigFlags_IsSRGB = 1 << 20,
    ImGuiConfigFlags_IsTouchScreen = 1 << 21
}ImGuiConfigFlags_;
typedef enum {
    ImGuiBackendFlags_None = 0,
    ImGuiBackendFlags_HasGamepad = 1 << 0,
    ImGuiBackendFlags_HasMouseCursors = 1 << 1,
    ImGuiBackendFlags_HasSetMousePos = 1 << 2,
    ImGuiBackendFlags_RendererHasVtxOffset = 1 << 3
}ImGuiBackendFlags_;
typedef enum {
    ImGuiCol_Text,
    ImGuiCol_TextDisabled,
    ImGuiCol_WindowBg,
    ImGuiCol_ChildBg,
    ImGuiCol_PopupBg,
    ImGuiCol_Border,
    ImGuiCol_BorderShadow,
    ImGuiCol_FrameBg,
    ImGuiCol_FrameBgHovered,
    ImGuiCol_FrameBgActive,
    ImGuiCol_TitleBg,
    ImGuiCol_TitleBgActive,
    ImGuiCol_TitleBgCollapsed,
    ImGuiCol_MenuBarBg,
    ImGuiCol_ScrollbarBg,
    ImGuiCol_ScrollbarGrab,
    ImGuiCol_ScrollbarGrabHovered,
    ImGuiCol_ScrollbarGrabActive,
    ImGuiCol_CheckMark,
    ImGuiCol_SliderGrab,
    ImGuiCol_SliderGrabActive,
    ImGuiCol_Button,
    ImGuiCol_ButtonHovered,
    ImGuiCol_ButtonActive,
    ImGuiCol_Header,
    ImGuiCol_HeaderHovered,
    ImGuiCol_HeaderActive,
    ImGuiCol_Separator,
    ImGuiCol_SeparatorHovered,
    ImGuiCol_SeparatorActive,
    ImGuiCol_ResizeGrip,
    ImGuiCol_ResizeGripHovered,
    ImGuiCol_ResizeGripActive,
    ImGuiCol_Tab,
    ImGuiCol_TabHovered,
    ImGuiCol_TabActive,
    ImGuiCol_TabUnfocused,
    ImGuiCol_TabUnfocusedActive,
    ImGuiCol_PlotLines,
    ImGuiCol_PlotLinesHovered,
    ImGuiCol_PlotHistogram,
    ImGuiCol_PlotHistogramHovered,
    ImGuiCol_TextSelectedBg,
    ImGuiCol_DragDropTarget,
    ImGuiCol_NavHighlight,
    ImGuiCol_NavWindowingHighlight,
    ImGuiCol_NavWindowingDimBg,
    ImGuiCol_ModalWindowDimBg,
    ImGuiCol_COUNT
}ImGuiCol_;
typedef enum {
    ImGuiStyleVar_Alpha,
    ImGuiStyleVar_WindowPadding,
    ImGuiStyleVar_WindowRounding,
    ImGuiStyleVar_WindowBorderSize,
    ImGuiStyleVar_WindowMinSize,
    ImGuiStyleVar_WindowTitleAlign,
    ImGuiStyleVar_ChildRounding,
    ImGuiStyleVar_ChildBorderSize,
    ImGuiStyleVar_PopupRounding,
    ImGuiStyleVar_PopupBorderSize,
    ImGuiStyleVar_FramePadding,
    ImGuiStyleVar_FrameRounding,
    ImGuiStyleVar_FrameBorderSize,
    ImGuiStyleVar_ItemSpacing,
    ImGuiStyleVar_ItemInnerSpacing,
    ImGuiStyleVar_IndentSpacing,
    ImGuiStyleVar_ScrollbarSize,
    ImGuiStyleVar_ScrollbarRounding,
    ImGuiStyleVar_GrabMinSize,
    ImGuiStyleVar_GrabRounding,
    ImGuiStyleVar_TabRounding,
    ImGuiStyleVar_ButtonTextAlign,
    ImGuiStyleVar_SelectableTextAlign,
    ImGuiStyleVar_COUNT
}ImGuiStyleVar_;
typedef enum {
    ImGuiColorEditFlags_None = 0,
    ImGuiColorEditFlags_NoAlpha = 1 << 1,
    ImGuiColorEditFlags_NoPicker = 1 << 2,
    ImGuiColorEditFlags_NoOptions = 1 << 3,
    ImGuiColorEditFlags_NoSmallPreview = 1 << 4,
    ImGuiColorEditFlags_NoInputs = 1 << 5,
    ImGuiColorEditFlags_NoTooltip = 1 << 6,
    ImGuiColorEditFlags_NoLabel = 1 << 7,
    ImGuiColorEditFlags_NoSidePreview = 1 << 8,
    ImGuiColorEditFlags_NoDragDrop = 1 << 9,
    ImGuiColorEditFlags_AlphaBar = 1 << 16,
    ImGuiColorEditFlags_AlphaPreview = 1 << 17,
    ImGuiColorEditFlags_AlphaPreviewHalf= 1 << 18,
    ImGuiColorEditFlags_HDR = 1 << 19,
    ImGuiColorEditFlags_DisplayRGB = 1 << 20,
    ImGuiColorEditFlags_DisplayHSV = 1 << 21,
    ImGuiColorEditFlags_DisplayHex = 1 << 22,
    ImGuiColorEditFlags_Uint8 = 1 << 23,
    ImGuiColorEditFlags_Float = 1 << 24,
    ImGuiColorEditFlags_PickerHueBar = 1 << 25,
    ImGuiColorEditFlags_PickerHueWheel = 1 << 26,
    ImGuiColorEditFlags_InputRGB = 1 << 27,
    ImGuiColorEditFlags_InputHSV = 1 << 28,
    ImGuiColorEditFlags__OptionsDefault = ImGuiColorEditFlags_Uint8|ImGuiColorEditFlags_DisplayRGB|ImGuiColorEditFlags_InputRGB|ImGuiColorEditFlags_PickerHueBar,
    ImGuiColorEditFlags__DisplayMask = ImGuiColorEditFlags_DisplayRGB|ImGuiColorEditFlags_DisplayHSV|ImGuiColorEditFlags_DisplayHex,
    ImGuiColorEditFlags__DataTypeMask = ImGuiColorEditFlags_Uint8|ImGuiColorEditFlags_Float,
    ImGuiColorEditFlags__PickerMask = ImGuiColorEditFlags_PickerHueWheel|ImGuiColorEditFlags_PickerHueBar,
    ImGuiColorEditFlags__InputMask = ImGuiColorEditFlags_InputRGB|ImGuiColorEditFlags_InputHSV
}ImGuiColorEditFlags_;
typedef enum {
    ImGuiMouseButton_Left = 0,
    ImGuiMouseButton_Right = 1,
    ImGuiMouseButton_Middle = 2,
    ImGuiMouseButton_COUNT = 5
}ImGuiMouseButton_;
typedef enum {
    ImGuiMouseCursor_None = -1,
    ImGuiMouseCursor_Arrow = 0,
    ImGuiMouseCursor_TextInput,
    ImGuiMouseCursor_ResizeAll,
    ImGuiMouseCursor_ResizeNS,
    ImGuiMouseCursor_ResizeEW,
    ImGuiMouseCursor_ResizeNESW,
    ImGuiMouseCursor_ResizeNWSE,
    ImGuiMouseCursor_Hand,
    ImGuiMouseCursor_NotAllowed,
    ImGuiMouseCursor_COUNT
}ImGuiMouseCursor_;
typedef enum {
    ImGuiCond_Always = 1 << 0,
    ImGuiCond_Once = 1 << 1,
    ImGuiCond_FirstUseEver = 1 << 2,
    ImGuiCond_Appearing = 1 << 3
}ImGuiCond_;
typedef struct ImGuiStyle
{
    float Alpha;
    ImVec2 WindowPadding;
    float WindowRounding;
    float WindowBorderSize;
    ImVec2 WindowMinSize;
    ImVec2 WindowTitleAlign;
    ImGuiDir WindowMenuButtonPosition;
    float ChildRounding;
    float ChildBorderSize;
    float PopupRounding;
    float PopupBorderSize;
    ImVec2 FramePadding;
    float FrameRounding;
    float FrameBorderSize;
    ImVec2 ItemSpacing;
    ImVec2 ItemInnerSpacing;
    ImVec2 TouchExtraPadding;
    float IndentSpacing;
    float ColumnsMinSpacing;
    float ScrollbarSize;
    float ScrollbarRounding;
    float GrabMinSize;
    float GrabRounding;
    float TabRounding;
    float TabBorderSize;
    ImGuiDir ColorButtonPosition;
    ImVec2 ButtonTextAlign;
    ImVec2 SelectableTextAlign;
    ImVec2 DisplayWindowPadding;
    ImVec2 DisplaySafeAreaPadding;
    float MouseCursorScale;
    bool AntiAliasedLines;
    bool AntiAliasedFill;
    float CurveTessellationTol;
    float CircleSegmentMaxError;
    ImVec4 Colors[ImGuiCol_COUNT];
} ImGuiStyle;
typedef struct ImGuiIO
{
    ImGuiConfigFlags ConfigFlags;
    ImGuiBackendFlags BackendFlags;
    ImVec2 DisplaySize;
    float DeltaTime;
    float IniSavingRate;
    const char* IniFilename;
    const char* LogFilename;
    float MouseDoubleClickTime;
    float MouseDoubleClickMaxDist;
    float MouseDragThreshold;
    int KeyMap[ImGuiKey_COUNT];
    float KeyRepeatDelay;
    float KeyRepeatRate;
    void* UserData;
    struct ImFontAtlas*Fonts;
    float FontGlobalScale;
    bool FontAllowUserScaling;
    struct ImFont* FontDefault;
    ImVec2 DisplayFramebufferScale;
    bool MouseDrawCursor;
    bool ConfigMacOSXBehaviors;
    bool ConfigInputTextCursorBlink;
    bool ConfigWindowsResizeFromEdges;
    bool ConfigWindowsMoveFromTitleBarOnly;
    float ConfigWindowsMemoryCompactTimer;
    const char* BackendPlatformName;
    const char* BackendRendererName;
    void* BackendPlatformUserData;
    void* BackendRendererUserData;
    void* BackendLanguageUserData;
    const char* (*GetClipboardTextFn)(void* user_data);
    void (*SetClipboardTextFn)(void* user_data, const char* text);
    void* ClipboardUserData;
    void (*ImeSetInputScreenPosFn)(int x, int y);
    void* ImeWindowHandle;
    void* RenderDrawListsFnUnused;
    ImVec2 MousePos;
    bool MouseDown[5];
    float MouseWheel;
    float MouseWheelH;
    bool KeyCtrl;
    bool KeyShift;
    bool KeyAlt;
    bool KeySuper;
    bool KeysDown[512];
    float NavInputs[ImGuiNavInput_COUNT];
    bool WantCaptureMouse;
    bool WantCaptureKeyboard;
    bool WantTextInput;
    bool WantSetMousePos;
    bool WantSaveIniSettings;
    bool NavActive;
    bool NavVisible;
    float Framerate;
    int MetricsRenderVertices;
    int MetricsRenderIndices;
    int MetricsRenderWindows;
    int MetricsActiveWindows;
    int MetricsActiveAllocations;
    ImVec2 MouseDelta;
    ImVec2 MousePosPrev;
    ImVec2 MouseClickedPos[5];
    double MouseClickedTime[5];
    bool MouseClicked[5];
    bool MouseDoubleClicked[5];
    bool MouseReleased[5];
    bool MouseDownOwned[5];
    bool MouseDownWasDoubleClick[5];
    float MouseDownDuration[5];
    float MouseDownDurationPrev[5];
    ImVec2 MouseDragMaxDistanceAbs[5];
    float MouseDragMaxDistanceSqr[5];
    float KeysDownDuration[512];
    float KeysDownDurationPrev[512];
    float NavInputsDownDuration[ImGuiNavInput_COUNT];
    float NavInputsDownDurationPrev[ImGuiNavInput_COUNT];
    ImVector_ImWchar InputQueueCharacters;
} ImGuiIO;
typedef struct ImGuiInputTextCallbackData
{
    ImGuiInputTextFlags EventFlag;
    ImGuiInputTextFlags Flags;
    void* UserData;
    ImWchar EventChar;
    ImGuiKey EventKey;
    char* Buf;
    int BufTextLen;
    int BufSize;
    bool BufDirty;
    int CursorPos;
    int SelectionStart;
    int SelectionEnd;
} ImGuiInputTextCallbackData;
typedef struct ImGuiSizeCallbackData
{
    void* UserData;
    ImVec2 Pos;
    ImVec2 CurrentSize;
    ImVec2 DesiredSize;
} ImGuiSizeCallbackData;
typedef struct ImGuiPayload
{
    void* Data;
    int DataSize;
    ImGuiID SourceId;
    ImGuiID SourceParentId;
    int DataFrameCount;
    char DataType[32+1];
    bool Preview;
    bool Delivery;
} ImGuiPayload;
typedef struct ImGuiOnceUponAFrame
{
     int RefFrame;
} ImGuiOnceUponAFrame;
typedef struct ImGuiTextFilter
{
    char InputBuf[256];
    ImVector_ImGuiTextRange Filters;
    int CountGrep;
} ImGuiTextFilter;
typedef struct ImGuiTextBuffer
{
    ImVector_char Buf;
} ImGuiTextBuffer;
typedef struct ImGuiStorage
{
    ImVector_ImGuiStoragePair Data;
} ImGuiStorage;
typedef struct ImGuiListClipper
{
    int DisplayStart, DisplayEnd;
    int ItemsCount;
    int StepNo;
    float ItemsHeight;
    float StartPosY;
} ImGuiListClipper;
typedef struct ImColor
{
    ImVec4 Value;
} ImColor;
typedef struct ImDrawCmd
{
    unsigned int ElemCount;
    ImVec4 ClipRect;
    ImTextureID TextureId;
    unsigned int VtxOffset;
    unsigned int IdxOffset;
    ImDrawCallback UserCallback;
    void* UserCallbackData;
} ImDrawCmd;
typedef struct ImDrawVert
{
    ImVec2 pos;
    ImVec2 uv;
    ImU32 col;
} ImDrawVert;
typedef struct ImDrawChannel
{
    ImVector_ImDrawCmd _CmdBuffer;
    ImVector_ImDrawIdx _IdxBuffer;
} ImDrawChannel;
typedef struct ImDrawListSplitter
{
    int _Current;
    int _Count;
    ImVector_ImDrawChannel _Channels;
} ImDrawListSplitter;
typedef enum {
    ImDrawCornerFlags_None = 0,
    ImDrawCornerFlags_TopLeft = 1 << 0,
    ImDrawCornerFlags_TopRight = 1 << 1,
    ImDrawCornerFlags_BotLeft = 1 << 2,
    ImDrawCornerFlags_BotRight = 1 << 3,
    ImDrawCornerFlags_Top = ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_TopRight,
    ImDrawCornerFlags_Bot = ImDrawCornerFlags_BotLeft | ImDrawCornerFlags_BotRight,
    ImDrawCornerFlags_Left = ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotLeft,
    ImDrawCornerFlags_Right = ImDrawCornerFlags_TopRight | ImDrawCornerFlags_BotRight,
    ImDrawCornerFlags_All = 0xF
}ImDrawCornerFlags_;
typedef enum {
    ImDrawListFlags_None = 0,
    ImDrawListFlags_AntiAliasedLines = 1 << 0,
    ImDrawListFlags_AntiAliasedFill = 1 << 1,
    ImDrawListFlags_AllowVtxOffset = 1 << 2
}ImDrawListFlags_;
typedef struct ImDrawList
{
    ImVector_ImDrawCmd CmdBuffer;
    ImVector_ImDrawIdx IdxBuffer;
    ImVector_ImDrawVert VtxBuffer;
    ImDrawListFlags Flags;
    const struct ImDrawListSharedData* _Data;
    const char* _OwnerName;
    unsigned int _VtxCurrentOffset;
    unsigned int _VtxCurrentIdx;
    ImDrawVert* _VtxWritePtr;
    ImDrawIdx* _IdxWritePtr;
    ImVector_ImVec4 _ClipRectStack;
    ImVector_ImTextureID _TextureIdStack;
    ImVector_ImVec2 _Path;
    ImDrawListSplitter _Splitter;
} ImDrawList;
typedef struct ImDrawData
{
    bool Valid;
    ImDrawList** CmdLists;
    int CmdListsCount;
    int TotalIdxCount;
    int TotalVtxCount;
    ImVec2 DisplayPos;
    ImVec2 DisplaySize;
    ImVec2 FramebufferScale;
} ImDrawData;
typedef struct ImFontConfig
{
    void* FontData;
    int FontDataSize;
    bool FontDataOwnedByAtlas;
    int FontNo;
    float SizePixels;
    int OversampleH;
    int OversampleV;
    bool PixelSnapH;
    ImVec2 GlyphExtraSpacing;
    ImVec2 GlyphOffset;
    const ImWchar* GlyphRanges;
    float GlyphMinAdvanceX;
    float GlyphMaxAdvanceX;
    bool MergeMode;
    unsigned int RasterizerFlags;
    float RasterizerMultiply;
    ImWchar EllipsisChar;
    char Name[40];
    struct ImFont* DstFont;
} ImFontConfig;
typedef struct ImFontGlyph
{
    ImWchar Codepoint;
    float AdvanceX;
    float X0, Y0, X1, Y1;
    float U0, V0, U1, V1;
} ImFontGlyph;
typedef struct ImFontGlyphRangesBuilder
{
    ImVector_ImU32 UsedChars;
} ImFontGlyphRangesBuilder;
typedef struct ImFontAtlasCustomRect
{
    unsigned int ID;
    unsigned short Width, Height;
    unsigned short X, Y;
    float GlyphAdvanceX;
    ImVec2 GlyphOffset;
    struct ImFont* Font;
} ImFontAtlasCustomRect;
typedef enum {
    ImFontAtlasFlags_None = 0,
    ImFontAtlasFlags_NoPowerOfTwoHeight = 1 << 0,
    ImFontAtlasFlags_NoMouseCursors = 1 << 1
}ImFontAtlasFlags_;
typedef struct ImFontAtlas
{
    bool Locked;
    ImFontAtlasFlags Flags;
    ImTextureID TexID;
    int TexDesiredWidth;
    int TexGlyphPadding;
    unsigned char* TexPixelsAlpha8;
    unsigned int* TexPixelsRGBA32;
    int TexWidth;
    int TexHeight;
    ImVec2 TexUvScale;
    ImVec2 TexUvWhitePixel;
    ImVector_ImFontPtr Fonts;
    ImVector_ImFontAtlasCustomRect CustomRects;
    ImVector_ImFontConfig ConfigData;
    int CustomRectIds[1];
} ImFontAtlas;
typedef struct ImFont
{
    ImVector_float IndexAdvanceX;
    float FallbackAdvanceX;
    float FontSize;
    ImVector_ImWchar IndexLookup;
    ImVector_ImFontGlyph Glyphs;
    const ImFontGlyph* FallbackGlyph;
    ImVec2 DisplayOffset;
    ImFontAtlas* ContainerAtlas;
    const ImFontConfig* ConfigData;
    short ConfigDataCount;
    ImWchar FallbackChar;
    ImWchar EllipsisChar;
    bool DirtyLookupTables;
    float Scale;
    float Ascent, Descent;
    int MetricsTotalSurface;
} ImFont;

typedef struct ImGuiTextRange
{
        const char* b;
        const char* e;
} ImGuiTextRange;

typedef struct ImGuiStoragePair
{
        ImGuiID key;
        union { int val_i; float val_f; void* val_p; };
} ImGuiStoragePair;

struct ImGuiContext* igCreateContext(ImFontAtlas* shared_font_atlas);
void igDestroyContext(ImGuiContext* ctx);
struct ImGuiContext* igGetCurrentContext(void);
void igSetCurrentContext(struct ImGuiContext* ctx);
bool igDebugCheckVersionAndDataLayout(const char* version_str,usize sz_io,usize sz_style,usize sz_vec2,usize sz_vec4,usize sz_drawvert,usize sz_drawidx);
ImGuiIO* igGetIO(void);
ImGuiStyle* igGetStyle(void);
void igNewFrame(void);
void igEndFrame(void);
void igRender(void);
ImDrawData* igGetDrawData(void);
void igShowDemoWindow(bool* p_open);
void igShowAboutWindow(bool* p_open);
void igShowMetricsWindow(bool* p_open);
void igShowStyleEditor(ImGuiStyle* ref);
bool igShowStyleSelector(const char* label);
void igShowFontSelector(const char* label);
void igShowUserGuide(void);
const char* igGetVersion(void);
void igStyleColorsDark(ImGuiStyle* dst);
void igStyleColorsClassic(ImGuiStyle* dst);
void igStyleColorsLight(ImGuiStyle* dst);
bool igBegin(const char* name,bool* p_open,ImGuiWindowFlags flags);
void igEnd(void);
bool igBeginChildStr(const char* str_id,const ImVec2 size,bool border,ImGuiWindowFlags flags);
bool igBeginChildID(ImGuiID id,const ImVec2 size,bool border,ImGuiWindowFlags flags);
void igEndChild(void);
bool igIsWindowAppearing(void);
bool igIsWindowCollapsed(void);
bool igIsWindowFocused(ImGuiFocusedFlags flags);
bool igIsWindowHovered(ImGuiHoveredFlags flags);
ImDrawList* igGetWindowDrawList(void);
void igGetWindowPos(ImVec2 *pOut);
void igGetWindowSize(ImVec2 *pOut);
float igGetWindowWidth(void);
float igGetWindowHeight(void);
void igSetNextWindowPos(const ImVec2 pos,ImGuiCond cond,const ImVec2 pivot);
void igSetNextWindowSize(const ImVec2 size,ImGuiCond cond);
void igSetNextWindowSizeConstraints(const ImVec2 size_min,const ImVec2 size_max,ImGuiSizeCallback custom_callback,void* custom_callback_data);
void igSetNextWindowContentSize(const ImVec2 size);
void igSetNextWindowCollapsed(bool collapsed,ImGuiCond cond);
void igSetNextWindowFocus(void);
void igSetNextWindowBgAlpha(float alpha);
void igSetWindowPosVec2(const ImVec2 pos,ImGuiCond cond);
void igSetWindowSizeVec2(const ImVec2 size,ImGuiCond cond);
void igSetWindowCollapsedBool(bool collapsed,ImGuiCond cond);
void igSetWindowFocusNil(void);
void igSetWindowFontScale(float scale);
void igSetWindowPosStr(const char* name,const ImVec2 pos,ImGuiCond cond);
void igSetWindowSizeStr(const char* name,const ImVec2 size,ImGuiCond cond);
void igSetWindowCollapsedStr(const char* name,bool collapsed,ImGuiCond cond);
void igSetWindowFocusStr(const char* name);
void igGetContentRegionMax(ImVec2 *pOut);
void igGetContentRegionAvail(ImVec2 *pOut);
void igGetWindowContentRegionMin(ImVec2 *pOut);
void igGetWindowContentRegionMax(ImVec2 *pOut);
float igGetWindowContentRegionWidth(void);
float igGetScrollX(void);
float igGetScrollY(void);
float igGetScrollMaxX(void);
float igGetScrollMaxY(void);
void igSetScrollX(float scroll_x);
void igSetScrollY(float scroll_y);
void igSetScrollHereX(float center_x_ratio);
void igSetScrollHereY(float center_y_ratio);
void igSetScrollFromPosX(float local_x,float center_x_ratio);
void igSetScrollFromPosY(float local_y,float center_y_ratio);
void igPushFont(ImFont* font);
void igPopFont(void);
void igPushStyleColorU32(ImGuiCol idx,ImU32 col);
void igPushStyleColorVec4(ImGuiCol idx,const ImVec4 col);
void igPopStyleColor(int count);
void igPushStyleVarFloat(ImGuiStyleVar idx,float val);
void igPushStyleVarVec2(ImGuiStyleVar idx,const ImVec2 val);
void igPopStyleVar(int count);
const ImVec4* igGetStyleColorVec4(ImGuiCol idx);
ImFont* igGetFont(void);
float igGetFontSize(void);
void igGetFontTexUvWhitePixel(ImVec2 *pOut);
ImU32 igGetColorU32Col(ImGuiCol idx,float alpha_mul);
ImU32 igGetColorU32Vec4(const ImVec4 col);
ImU32 igGetColorU32U32(ImU32 col);
void igPushItemWidth(float item_width);
void igPopItemWidth(void);
void igSetNextItemWidth(float item_width);
float igCalcItemWidth(void);
void igPushTextWrapPos(float wrap_local_pos_x);
void igPopTextWrapPos(void);
void igPushAllowKeyboardFocus(bool allow_keyboard_focus);
void igPopAllowKeyboardFocus(void);
void igPushButtonRepeat(bool repeat);
void igPopButtonRepeat(void);
void igSeparator(void);
void igSameLine(float offset_from_start_x,float spacing);
void igNewLine(void);
void igSpacing(void);
void igDummy(const ImVec2 size);
void igIndent(float indent_w);
void igUnindent(float indent_w);
void igBeginGroup(void);
void igEndGroup(void);
void igGetCursorPos(ImVec2 *pOut);
float igGetCursorPosX(void);
float igGetCursorPosY(void);
void igSetCursorPos(const ImVec2 local_pos);
void igSetCursorPosX(float local_x);
void igSetCursorPosY(float local_y);
void igGetCursorStartPos(ImVec2 *pOut);
void igGetCursorScreenPos(ImVec2 *pOut);
void igSetCursorScreenPos(const ImVec2 pos);
void igAlignTextToFramePadding(void);
float igGetTextLineHeight(void);
float igGetTextLineHeightWithSpacing(void);
float igGetFrameHeight(void);
float igGetFrameHeightWithSpacing(void);
void igPushIDStr(const char* str_id);
void igPushIDStrStr(const char* str_id_begin,const char* str_id_end);
void igPushIDPtr(const void* ptr_id);
void igPushIDInt(int int_id);
void igPopID(void);
ImGuiID igGetIDStr(const char* str_id);
ImGuiID igGetIDStrStr(const char* str_id_begin,const char* str_id_end);
ImGuiID igGetIDPtr(const void* ptr_id);
void igTextUnformatted(const char* text,const char* text_end);
void igText(const char* fmt,...);
void igTextV(const char* fmt, VaList args);
void igTextColored(const ImVec4 col,const char* fmt,...);
void igTextColoredV(const ImVec4 col,const char* fmt, VaList args);
void igTextDisabled(const char* fmt,...);
void igTextDisabledV(const char* fmt, VaList args);
void igTextWrapped(const char* fmt,...);
void igTextWrappedV(const char* fmt, VaList args);
void igLabelText(const char* label,const char* fmt,...);
void igLabelTextV(const char* label,const char* fmt, VaList args);
void igBulletText(const char* fmt,...);
void igBulletTextV(const char* fmt, VaList args);
bool igButton(const char* label);
bool igSmallButton(const char* label);
bool igInvisibleButton(const char* str_id,const ImVec2 size);
bool igArrowButton(const char* str_id,ImGuiDir dir);
void igImage(ImTextureID user_texture_id,const ImVec2 size,const ImVec2 uv0,const ImVec2 uv1,const ImVec4 tint_col,const ImVec4 border_col);
bool igImageButton(ImTextureID user_texture_id,const ImVec2 size,const ImVec2 uv0,const ImVec2 uv1,int frame_padding,const ImVec4 bg_col,const ImVec4 tint_col);
bool igCheckbox(const char* label,bool* v);
bool igCheckboxFlags(const char* label,unsigned int* flags,unsigned int flags_value);
bool igRadioButtonBool(const char* label,bool active);
bool igRadioButtonIntPtr(const char* label,int* v,int v_button);
void igProgressBar(float fraction,const ImVec2 size_arg,const char* overlay);
void igBullet(void);
bool igBeginCombo(const char* label,const char* preview_value,ImGuiComboFlags flags);
void igEndCombo(void);
bool igComboStr_arr(const char* label,int* current_item,const char* const items[],int items_count,int popup_max_height_in_items);
bool igComboStr(const char* label,int* current_item,const char* items_separated_by_zeros,int popup_max_height_in_items);
bool igComboFnPtr(const char* label,int* current_item,bool(*items_getter)(void* data,int idx,const char** out_text),void* data,int items_count,int popup_max_height_in_items);
bool igDragFloat(const char* label,float* v,float v_speed,float v_min,float v_max,const char* format,float power);
bool igDragFloat2(const char* label,float v[2],float v_speed,float v_min,float v_max,const char* format,float power);
bool igDragFloat3(const char* label,float v[3],float v_speed,float v_min,float v_max,const char* format,float power);
bool igDragFloat4(const char* label,float v[4],float v_speed,float v_min,float v_max,const char* format,float power);
bool igDragFloatRange2(const char* label,float* v_current_min,float* v_current_max,float v_speed,float v_min,float v_max,const char* format,const char* format_max,float power);
bool igDragInt(const char* label,int* v,float v_speed,int v_min,int v_max,const char* format);
bool igDragInt2(const char* label,int v[2],float v_speed,int v_min,int v_max,const char* format);
bool igDragInt3(const char* label,int v[3],float v_speed,int v_min,int v_max,const char* format);
bool igDragInt4(const char* label,int v[4],float v_speed,int v_min,int v_max,const char* format);
bool igDragIntRange2(const char* label,int* v_current_min,int* v_current_max,float v_speed,int v_min,int v_max,const char* format,const char* format_max);
bool igDragScalar(const char* label,ImGuiDataType data_type,void* p_data,float v_speed,const void* p_min,const void* p_max,const char* format,float power);
bool igDragScalarN(const char* label,ImGuiDataType data_type,void* p_data,int components,float v_speed,const void* p_min,const void* p_max,const char* format,float power);
bool igSliderFloat(const char* label,float* v,float v_min,float v_max);
bool igSliderFloat2(const char* label,float v[2],float v_min,float v_max);
bool igSliderFloat3(const char* label,float v[3],float v_min,float v_max);
bool igSliderFloat4(const char* label,float v[4],float v_min,float v_max);
bool igSliderAngle(const char* label,float* v_rad,float v_degrees_min,float v_degrees_max,const char* format);
bool igSliderInt(const char* label,int* v,int v_min,int v_max,const char* format);
bool igSliderInt2(const char* label,int v[2],int v_min,int v_max,const char* format);
bool igSliderInt3(const char* label,int v[3],int v_min,int v_max,const char* format);
bool igSliderInt4(const char* label,int v[4],int v_min,int v_max,const char* format);
bool igSliderScalar(const char* label,ImGuiDataType data_type,void* p_data,const void* p_min,const void* p_max,const char* format,float power);
bool igSliderScalarN(const char* label,ImGuiDataType data_type,void* p_data,int components,const void* p_min,const void* p_max,const char* format,float power);
bool igVSliderFloat(const char* label,const ImVec2 size,float* v,float v_min,float v_max,const char* format,float power);
bool igVSliderInt(const char* label,const ImVec2 size,int* v,int v_min,int v_max,const char* format);
bool igVSliderScalar(const char* label,const ImVec2 size,ImGuiDataType data_type,void* p_data,const void* p_min,const void* p_max,const char* format,float power);
bool igInputText(const char* label,char* buf,usize buf_size,ImGuiInputTextFlags flags,ImGuiInputTextCallback callback,void* user_data);
bool igInputTextMultiline(const char* label,char* buf,usize buf_size,const ImVec2 size,ImGuiInputTextFlags flags,ImGuiInputTextCallback callback,void* user_data);
bool igInputTextWithHint(const char* label,const char* hint,char* buf,usize buf_size,ImGuiInputTextFlags flags,ImGuiInputTextCallback callback,void* user_data);
bool igInputFloat(const char* label,float* v,float step,float step_fast,const char* format,ImGuiInputTextFlags flags);
bool igInputFloat2(const char* label,float v[2],const char* format,ImGuiInputTextFlags flags);
bool igInputFloat3(const char* label,float v[3],const char* format,ImGuiInputTextFlags flags);
bool igInputFloat4(const char* label,float v[4],const char* format,ImGuiInputTextFlags flags);
bool igInputInt(const char* label,int* v,int step,int step_fast,ImGuiInputTextFlags flags);
bool igInputInt2(const char* label,int v[2],ImGuiInputTextFlags flags);
bool igInputInt3(const char* label,int v[3],ImGuiInputTextFlags flags);
bool igInputInt4(const char* label,int v[4],ImGuiInputTextFlags flags);
bool igInputDouble(const char* label,double* v,double step,double step_fast,const char* format,ImGuiInputTextFlags flags);
bool igInputScalar(const char* label,ImGuiDataType data_type,void* p_data,const void* p_step,const void* p_step_fast,const char* format,ImGuiInputTextFlags flags);
bool igInputScalarN(const char* label,ImGuiDataType data_type,void* p_data,int components,const void* p_step,const void* p_step_fast,const char* format,ImGuiInputTextFlags flags);
bool igColorEdit3(const char* label,float col[3],ImGuiColorEditFlags flags);
bool igColorEdit4(const char* label,float col[4],ImGuiColorEditFlags flags);
bool igColorPicker3(const char* label,float col[3],ImGuiColorEditFlags flags);
bool igColorPicker4(const char* label,float col[4],ImGuiColorEditFlags flags,const float* ref_col);
bool igColorButton(const char* desc_id,const ImVec4 col,ImGuiColorEditFlags flags,ImVec2 size);
void igSetColorEditOptions(ImGuiColorEditFlags flags);
bool igTreeNodeStr(const char* label);
bool igTreeNodeStrStr(const char* str_id,const char* fmt,...);
bool igTreeNodePtr(const void* ptr_id,const char* fmt,...);
bool igTreeNodeVStr(const char* str_id,const char* fmt, VaList args);
bool igTreeNodeVPtr(const void* ptr_id,const char* fmt, VaList args);
bool igTreeNodeExStr(const char* label,ImGuiTreeNodeFlags flags);
bool igTreeNodeExStrStr(const char* str_id,ImGuiTreeNodeFlags flags,const char* fmt,...);
bool igTreeNodeExPtr(const void* ptr_id,ImGuiTreeNodeFlags flags,const char* fmt,...);
bool igTreeNodeExVStr(const char* str_id,ImGuiTreeNodeFlags flags,const char* fmt,VaList args);
bool igTreeNodeExVPtr(const void* ptr_id,ImGuiTreeNodeFlags flags,const char* fmt,VaList args);
void igTreePushStr(const char* str_id);
void igTreePushPtr(const void* ptr_id);
void igTreePop(void);
float igGetTreeNodeToLabelSpacing(void);
bool igCollapsingHeader1(const char* label);
bool igCollapsingHeader2(const char* label,bool* p_open);
void igSetNextItemOpen(bool is_open,ImGuiCond cond);
bool igSelectableBool(const char* label,bool selected,ImGuiSelectableFlags flags,const ImVec2 size);
bool igSelectableBoolPtr(const char* label,bool* p_selected,ImGuiSelectableFlags flags,const ImVec2 size);
bool igListBoxStr_arr(const char* label,int* current_item,const char* const items[],int items_count,int height_in_items);
bool igListBoxFnPtr(const char* label,int* current_item,bool(*items_getter)(void* data,int idx,const char** out_text),void* data,int items_count,int height_in_items);
bool igListBoxHeaderVec2(const char* label,const ImVec2 size);
bool igListBoxHeaderInt(const char* label,int items_count,int height_in_items);
void igListBoxFooter(void);
void igPlotLinesFloatPtr(const char* label,const float* values,int values_count,int values_offset,const char* overlay_text,float scale_min,float scale_max,ImVec2 graph_size,int stride);
void igPlotLinesFnPtr(const char* label,float(*values_getter)(void* data,int idx),void* data,int values_count,int values_offset,const char* overlay_text,float scale_min,float scale_max,ImVec2 graph_size);
void igPlotHistogramFloatPtr(const char* label,const float* values,int values_count,int values_offset,const char* overlay_text,float scale_min,float scale_max,ImVec2 graph_size,int stride);
void igPlotHistogramFnPtr(const char* label,float(*values_getter)(void* data,int idx),void* data,int values_count,int values_offset,const char* overlay_text,float scale_min,float scale_max,ImVec2 graph_size);
void igValueBool(const char* prefix,bool b);
void igValueInt(const char* prefix,int v);
void igValueUint(const char* prefix,unsigned int v);
void igValueFloat(const char* prefix, float v);
void igValueFloat2(const char* prefix, float v[2]);
void igValueFloat3(const char* prefix, float v[3]);
void igValueFloat4(const char* prefix, float v[4]);
bool igBeginMenuBar(void);
void igEndMenuBar(void);
bool igBeginMainMenuBar(void);
void igEndMainMenuBar(void);
bool igBeginMenu(const char* label,bool enabled);
void igEndMenu(void);
bool igMenuItem(const char* label);
bool igMenuItemBool(const char* label,const char* shortcut,bool selected,bool enabled);
bool igMenuItemBoolPtr(const char* label,const char* shortcut,bool* p_selected,bool enabled);
void igBeginTooltip(void);
void igEndTooltip(void);
void igSetTooltip(const char* fmt,...);
void igSetTooltipV(const char* fmt,VaList args);
void igOpenPopup(const char* str_id);
bool igBeginPopup(const char* str_id,ImGuiWindowFlags flags);
bool igBeginPopupContextItem(const char* str_id,ImGuiMouseButton mouse_button);
bool igBeginPopupContextWindow(const char* str_id,ImGuiMouseButton mouse_button,bool also_over_items);
bool igBeginPopupContextVoid(const char* str_id,ImGuiMouseButton mouse_button);
bool igBeginPopupModal(const char* name,bool* p_open,ImGuiWindowFlags flags);
void igEndPopup(void);
bool igOpenPopupOnItemClick(const char* str_id,ImGuiMouseButton mouse_button);
bool igIsPopupOpen(const char* str_id);
void igCloseCurrentPopup(void);
void igColumns(int count); // 1 for ending scope
void igNextColumn(void);
int igGetColumnIndex(void);
float igGetColumnWidth(int column_index);
void igSetColumnWidth(int column_index,float width);
float igGetColumnOffset(int column_index);
void igSetColumnOffset(int column_index,float offset_x);
int igGetColumnsCount(void);
bool igBeginTabBar(const char* str_id,ImGuiTabBarFlags flags);
void igEndTabBar(void);
bool igBeginTabItem(const char* label,bool* p_open,ImGuiTabItemFlags flags);
void igEndTabItem(void);
void igSetTabItemClosed(const char* tab_or_docked_window_label);
void igLogToTTY(int auto_open_depth);
void igLogToFile(int auto_open_depth,const char* filename);
void igLogToClipboard(int auto_open_depth);
void igLogFinish(void);
void igLogButtons(void);
bool igBeginDragDropSource(ImGuiDragDropFlags flags);
bool igSetDragDropPayload(const char* type,const void* data,usize sz,ImGuiCond cond);
void igEndDragDropSource(void);
bool igBeginDragDropTarget(void);
const ImGuiPayload* igAcceptDragDropPayload(const char* type,ImGuiDragDropFlags flags);
void igEndDragDropTarget(void);
const ImGuiPayload* igGetDragDropPayload(void);
void igPushClipRect(const ImVec2 clip_rect_min,const ImVec2 clip_rect_max,bool intersect_with_current_clip_rect);
void igPopClipRect(void);
void igSetItemDefaultFocus(void);
void igSetKeyboardFocusHere(int offset);
bool igIsItemHovered(ImGuiHoveredFlags flags);
bool igIsItemActive(void);
bool igIsItemFocused(void);
bool igIsItemClicked(ImGuiMouseButton mouse_button);
bool igIsItemVisible(void);
bool igIsItemEdited(void);
bool igIsItemActivated(void);
bool igIsItemDeactivated(void);
bool igIsItemDeactivatedAfterEdit(void);
bool igIsItemToggledOpen(void);
bool igIsAnyItemHovered(void);
bool igIsAnyItemActive(void);
bool igIsAnyItemFocused(void);
void igGetItemRectMin(ImVec2 *pOut);
void igGetItemRectMax(ImVec2 *pOut);
void igGetItemRectSize(ImVec2 *pOut);
void igSetItemAllowOverlap(void);
bool igIsRectVisibleNil(const ImVec2 size);
bool igIsRectVisibleVec2(const ImVec2 rect_min,const ImVec2 rect_max);
double igGetTime(void);
int igGetFrameCount(void);
ImDrawList* igGetBackgroundDrawList(void);
ImDrawList* igGetForegroundDrawList(void);
ImDrawListSharedData* igGetDrawListSharedData(void);
const char* igGetStyleColorName(ImGuiCol idx);
void igSetStateStorage(ImGuiStorage* storage);
ImGuiStorage* igGetStateStorage(void);
void igCalcTextSize(ImVec2 *pOut,const char* text,const char* text_end,bool hide_text_after_double_hash,float wrap_width);
void igCalcListClipping(int items_count,float items_height,int* out_items_display_start,int* out_items_display_end);
bool igBeginChildFrame(ImGuiID id,const ImVec2 size,ImGuiWindowFlags flags);
void igEndChildFrame(void);
void igColorConvertU32ToFloat4(ImVec4 *pOut,ImU32 in);
ImU32 igColorConvertFloat4ToU32(const ImVec4 in);
int igGetKeyIndex(ImGuiKey imgui_key);
bool igIsKeyDown(int user_key_index);
bool igIsKeyPressed(int user_key_index,bool repeat);
bool igIsKeyReleased(int user_key_index);
int igGetKeyPressedAmount(int key_index,float repeat_delay,float rate);
void igCaptureKeyboardFromApp(bool want_capture_keyboard_value);
bool igIsMouseDown(ImGuiMouseButton button);
bool igIsMouseClicked(ImGuiMouseButton button,bool repeat);
bool igIsMouseReleased(ImGuiMouseButton button);
bool igIsMouseDoubleClicked(ImGuiMouseButton button);
bool igIsMouseHoveringRect(const ImVec2 r_min,const ImVec2 r_max,bool clip);
bool igIsMousePosValid(const ImVec2* mouse_pos);
bool igIsAnyMouseDown(void);
void igGetMousePos(ImVec2 *pOut);
void igGetMousePosOnOpeningCurrentPopup(ImVec2 *pOut);
bool igIsMouseDragging(ImGuiMouseButton button,float lock_threshold);
void igGetMouseDragDelta(ImVec2 *pOut,ImGuiMouseButton button,float lock_threshold);
void igResetMouseDragDelta(ImGuiMouseButton button);
ImGuiMouseCursor igGetMouseCursor(void);
void igSetMouseCursor(ImGuiMouseCursor cursor_type);
void igCaptureMouseFromApp(bool want_capture_mouse_value);
const char* igGetClipboardText(void);
void igSetClipboardText(const char* text);
void igLoadIniSettingsFromDisk(const char* ini_filename);
void igLoadIniSettingsFromMemory(const char* ini_data,usize ini_size);
void igSaveIniSettingsToDisk(const char* ini_filename);
const char* igSaveIniSettingsToMemory(usize* out_ini_size);
void igSetAllocatorFunctions(void*(*alloc_func)(usize sz,void* user_data),void(*free_func)(void* ptr,void* user_data),void* user_data);
void* igMemAlloc(usize size);
void igMemFree(void* ptr);
void ImGuiStyle_ScaleAllSizes(ImGuiStyle* self,float scale_factor);
void ImGuiIO_AddInputCharacter(ImGuiIO* self,unsigned int c);
void ImGuiIO_AddInputCharactersUTF8(ImGuiIO* self,const char* str);
void ImGuiIO_ClearInputCharacters(ImGuiIO* self);
void ImGuiInputTextCallbackData_DeleteChars(ImGuiInputTextCallbackData* self,int pos,int bytes_count);
void ImGuiInputTextCallbackData_InsertChars(ImGuiInputTextCallbackData* self,int pos,const char* text,const char* text_end);
bool ImGuiInputTextCallbackData_HasSelection(ImGuiInputTextCallbackData* self);
void ImGuiPayload_Clear(ImGuiPayload* self);
bool ImGuiPayload_IsDataType(ImGuiPayload* self,const char* type);
bool ImGuiPayload_IsPreview(ImGuiPayload* self);
bool ImGuiPayload_IsDelivery(ImGuiPayload* self);
void ImColor_SetHSV(ImColor* self,float h,float s,float v,float a);
void ImColor_HSV(ImColor *pOut,ImColor* self,float h,float s,float v,float a);
void ImDrawListSplitter_Clear(ImDrawListSplitter* self);
void ImDrawListSplitter_ClearFreeMemory(ImDrawListSplitter* self);
void ImDrawListSplitter_Split(ImDrawListSplitter* self,ImDrawList* draw_list,int count);
void ImDrawListSplitter_Merge(ImDrawListSplitter* self,ImDrawList* draw_list);
void ImDrawListSplitter_SetCurrentChannel(ImDrawListSplitter* self,ImDrawList* draw_list,int channel_idx);
void ImDrawList_PushClipRect(ImDrawList* self,ImVec2 clip_rect_min,ImVec2 clip_rect_max,bool intersect_with_current_clip_rect);
void ImDrawList_PushClipRectFullScreen(ImDrawList* self);
void ImDrawList_PopClipRect(ImDrawList* self);
void ImDrawList_PushTextureID(ImDrawList* self,ImTextureID texture_id);
void ImDrawList_PopTextureID(ImDrawList* self);
void ImDrawList_GetClipRectMin(ImVec2 *pOut,ImDrawList* self);
void ImDrawList_GetClipRectMax(ImVec2 *pOut,ImDrawList* self);
void ImDrawList_AddLine(ImDrawList* self,const ImVec2 p1,const ImVec2 p2,ImU32 col,float thickness);
void ImDrawList_AddRect(ImDrawList* self,const ImVec2 p_min,const ImVec2 p_max,ImU32 col,float rounding,ImDrawCornerFlags rounding_corners,float thickness);
void ImDrawList_AddRectFilled(ImDrawList* self,const ImVec2 p_min,const ImVec2 p_max,ImU32 col,float rounding,ImDrawCornerFlags rounding_corners);
void ImDrawList_AddRectFilledMultiColor(ImDrawList* self,const ImVec2 p_min,const ImVec2 p_max,ImU32 col_upr_left,ImU32 col_upr_right,ImU32 col_bot_right,ImU32 col_bot_left);
void ImDrawList_AddQuad(ImDrawList* self,const ImVec2 p1,const ImVec2 p2,const ImVec2 p3,const ImVec2 p4,ImU32 col,float thickness);
void ImDrawList_AddQuadFilled(ImDrawList* self,const ImVec2 p1,const ImVec2 p2,const ImVec2 p3,const ImVec2 p4,ImU32 col);
void ImDrawList_AddTriangle(ImDrawList* self,const ImVec2 p1,const ImVec2 p2,const ImVec2 p3,ImU32 col,float thickness);
void ImDrawList_AddTriangleFilled(ImDrawList* self,const ImVec2 p1,const ImVec2 p2,const ImVec2 p3,ImU32 col);
void ImDrawList_AddCircle(ImDrawList* self,const ImVec2 center,float radius,ImU32 col,int num_segments,float thickness);
void ImDrawList_AddCircleFilled(ImDrawList* self,const ImVec2 center,float radius,ImU32 col,int num_segments);
void ImDrawList_AddNgon(ImDrawList* self,const ImVec2 center,float radius,ImU32 col,int num_segments,float thickness);
void ImDrawList_AddNgonFilled(ImDrawList* self,const ImVec2 center,float radius,ImU32 col,int num_segments);
void ImDrawList_AddTextVec2(ImDrawList* self,const ImVec2 pos,ImU32 col,const char* text_begin,const char* text_end);
void ImDrawList_AddTextFontPtr(ImDrawList* self,const ImFont* font,float font_size,const ImVec2 pos,ImU32 col,const char* text_begin,const char* text_end,float wrap_width,const ImVec4* cpu_fine_clip_rect);
void ImDrawList_AddPolyline(ImDrawList* self,const ImVec2* points,int num_points,ImU32 col,bool closed,float thickness);
void ImDrawList_AddConvexPolyFilled(ImDrawList* self,const ImVec2* points,int num_points,ImU32 col);
void ImDrawList_AddBezierCurve(ImDrawList* self,const ImVec2 p1,const ImVec2 p2,const ImVec2 p3,const ImVec2 p4,ImU32 col,float thickness,int num_segments);
void ImDrawList_AddImage(ImDrawList* self,ImTextureID user_texture_id,const ImVec2 p_min,const ImVec2 p_max,const ImVec2 uv_min,const ImVec2 uv_max,ImU32 col);
void ImDrawList_AddImageQuad(ImDrawList* self,ImTextureID user_texture_id,const ImVec2 p1,const ImVec2 p2,const ImVec2 p3,const ImVec2 p4,const ImVec2 uv1,const ImVec2 uv2,const ImVec2 uv3,const ImVec2 uv4,ImU32 col);
void ImDrawList_AddImageRounded(ImDrawList* self,ImTextureID user_texture_id,const ImVec2 p_min,const ImVec2 p_max,const ImVec2 uv_min,const ImVec2 uv_max,ImU32 col,float rounding,ImDrawCornerFlags rounding_corners);
void ImDrawList_PathClear(ImDrawList* self);
void ImDrawList_PathLineTo(ImDrawList* self,const ImVec2 pos);
void ImDrawList_PathLineToMergeDuplicate(ImDrawList* self,const ImVec2 pos);
void ImDrawList_PathFillConvex(ImDrawList* self,ImU32 col);
void ImDrawList_PathStroke(ImDrawList* self,ImU32 col,bool closed,float thickness);
void ImDrawList_PathArcTo(ImDrawList* self,const ImVec2 center,float radius,float a_min,float a_max,int num_segments);
void ImDrawList_PathArcToFast(ImDrawList* self,const ImVec2 center,float radius,int a_min_of_12,int a_max_of_12);
void ImDrawList_PathBezierCurveTo(ImDrawList* self,const ImVec2 p2,const ImVec2 p3,const ImVec2 p4,int num_segments);
void ImDrawList_PathRect(ImDrawList* self,const ImVec2 rect_min,const ImVec2 rect_max,float rounding,ImDrawCornerFlags rounding_corners);
void ImDrawList_AddCallback(ImDrawList* self,ImDrawCallback callback,void* callback_data);
void ImDrawList_AddDrawCmd(ImDrawList* self);
ImDrawList* ImDrawList_CloneOutput(ImDrawList* self);
void ImDrawList_ChannelsSplit(ImDrawList* self,int count);
void ImDrawList_ChannelsMerge(ImDrawList* self);
void ImDrawList_ChannelsSetCurrent(ImDrawList* self,int n);
void ImDrawList_Clear(ImDrawList* self);
void ImDrawList_ClearFreeMemory(ImDrawList* self);
void ImDrawList_PrimReserve(ImDrawList* self,int idx_count,int vtx_count);
void ImDrawList_PrimUnreserve(ImDrawList* self,int idx_count,int vtx_count);
void ImDrawList_PrimRect(ImDrawList* self,const ImVec2 a,const ImVec2 b,ImU32 col);
void ImDrawList_PrimRectUV(ImDrawList* self,const ImVec2 a,const ImVec2 b,const ImVec2 uv_a,const ImVec2 uv_b,ImU32 col);
void ImDrawList_PrimQuadUV(ImDrawList* self,const ImVec2 a,const ImVec2 b,const ImVec2 c,const ImVec2 d,const ImVec2 uv_a,const ImVec2 uv_b,const ImVec2 uv_c,const ImVec2 uv_d,ImU32 col);
void ImDrawList_PrimWriteVtx(ImDrawList* self,const ImVec2 pos,const ImVec2 uv,ImU32 col);
void ImDrawList_PrimWriteIdx(ImDrawList* self,ImDrawIdx idx);
void ImDrawList_PrimVtx(ImDrawList* self,const ImVec2 pos,const ImVec2 uv,ImU32 col);
void ImDrawList_UpdateClipRect(ImDrawList* self);
void ImDrawList_UpdateTextureID(ImDrawList* self);
void ImDrawData_Clear(ImDrawData* self);
void ImDrawData_DeIndexAllBuffers(ImDrawData* self);
void ImDrawData_ScaleClipRects(ImDrawData* self,const ImVec2 fb_scale);
void ImFontGlyphRangesBuilder_Clear(ImFontGlyphRangesBuilder* self);
bool ImFontGlyphRangesBuilder_GetBit(ImFontGlyphRangesBuilder* self,int n);
void ImFontGlyphRangesBuilder_SetBit(ImFontGlyphRangesBuilder* self,int n);
void ImFontGlyphRangesBuilder_AddChar(ImFontGlyphRangesBuilder* self,ImWchar c);
void ImFontGlyphRangesBuilder_AddText(ImFontGlyphRangesBuilder* self,const char* text,const char* text_end);
void ImFontGlyphRangesBuilder_AddRanges(ImFontGlyphRangesBuilder* self, const ImWchar* ranges);
bool ImFontAtlasCustomRect_IsPacked(ImFontAtlasCustomRect* self);
ImFont* ImFontAtlas_AddFont(ImFontAtlas* self,const ImFontConfig* font_cfg);
ImFont* ImFontAtlas_AddFontDefault(ImFontAtlas* self,const ImFontConfig* font_cfg);
ImFont* ImFontAtlas_AddFontFromFileTTF(ImFontAtlas* self,const char* filename,float size_pixels,const ImFontConfig* font_cfg,const ImWchar* glyph_ranges);
ImFont* ImFontAtlas_AddFontFromMemoryTTF(ImFontAtlas* self,void* font_data,int font_size,float size_pixels,const ImFontConfig* font_cfg,const ImWchar* glyph_ranges);
ImFont* ImFontAtlas_AddFontFromMemoryCompressedTTF(ImFontAtlas* self,const void* compressed_font_data,int compressed_font_size,float size_pixels,const ImFontConfig* font_cfg,const ImWchar* glyph_ranges);
ImFont* ImFontAtlas_AddFontFromMemoryCompressedBase85TTF(ImFontAtlas* self,const char* compressed_font_data_base85,float size_pixels,const ImFontConfig* font_cfg,const ImWchar* glyph_ranges);
void ImFontAtlas_ClearInputData(ImFontAtlas* self);
void ImFontAtlas_ClearTexData(ImFontAtlas* self);
void ImFontAtlas_ClearFonts(ImFontAtlas* self);
void ImFontAtlas_Clear(ImFontAtlas* self);
bool ImFontAtlas_Build(ImFontAtlas* self);
void ImFontAtlas_GetTexDataAsAlpha8(ImFontAtlas* self,unsigned char** out_pixels,int* out_width,int* out_height,int* out_bytes_per_pixel);
void ImFontAtlas_GetTexDataAsRGBA32(ImFontAtlas* self,unsigned char** out_pixels,int* out_width,int* out_height,int* out_bytes_per_pixel);
bool ImFontAtlas_IsBuilt(ImFontAtlas* self);
void ImFontAtlas_SetTexID(ImFontAtlas* self,ImTextureID id);
const ImWchar* ImFontAtlas_GetGlyphRangesDefault(ImFontAtlas* self);
const ImWchar* ImFontAtlas_GetGlyphRangesKorean(ImFontAtlas* self);
const ImWchar* ImFontAtlas_GetGlyphRangesJapanese(ImFontAtlas* self);
const ImWchar* ImFontAtlas_GetGlyphRangesChineseFull(ImFontAtlas* self);
const ImWchar* ImFontAtlas_GetGlyphRangesChineseSimplifiedCommon(ImFontAtlas* self);
const ImWchar* ImFontAtlas_GetGlyphRangesCyrillic(ImFontAtlas* self);
const ImWchar* ImFontAtlas_GetGlyphRangesThai(ImFontAtlas* self);
const ImWchar* ImFontAtlas_GetGlyphRangesVietnamese(ImFontAtlas* self);
int ImFontAtlas_AddCustomRectRegular(ImFontAtlas* self,unsigned int id,int width,int height);
int ImFontAtlas_AddCustomRectFontGlyph(ImFontAtlas* self,ImFont* font,ImWchar id,int width,int height,float advance_x,const ImVec2 offset);
const ImFontAtlasCustomRect* ImFontAtlas_GetCustomRectByIndex(ImFontAtlas* self,int index);
void ImFontAtlas_CalcCustomRectUV(ImFontAtlas* self,const ImFontAtlasCustomRect* rect,ImVec2* out_uv_min,ImVec2* out_uv_max);
bool ImFontAtlas_GetMouseCursorTexData(ImFontAtlas* self,ImGuiMouseCursor cursor,ImVec2* out_offset,ImVec2* out_size,ImVec2 out_uv_border[2],ImVec2 out_uv_fill[2]);
const ImFontGlyph* ImFont_FindGlyph(ImFont* self,ImWchar c);
const ImFontGlyph* ImFont_FindGlyphNoFallback(ImFont* self,ImWchar c);
float ImFont_GetCharAdvance(ImFont* self,ImWchar c);
bool ImFont_IsLoaded(ImFont* self);
const char* ImFont_GetDebugName(ImFont* self);
void ImFont_CalcTextSizeA(ImVec2 *pOut,ImFont* self,float size,float max_width,float wrap_width,const char* text_begin,const char* text_end,const char** remaining);
const char* ImFont_CalcWordWrapPositionA(ImFont* self,float scale,const char* text,const char* text_end,float wrap_width);
void ImFont_RenderChar(ImFont* self,ImDrawList* draw_list,float size,ImVec2 pos,ImU32 col,ImWchar c);
void ImFont_RenderText(ImFont* self,ImDrawList* draw_list,float size,ImVec2 pos,ImU32 col,const ImVec4 clip_rect,const char* text_begin,const char* text_end,float wrap_width,bool cpu_fine_clip);
void ImFont_BuildLookupTable(ImFont* self);
void ImFont_ClearOutputData(ImFont* self);
void ImFont_GrowIndex(ImFont* self,int new_size);
void ImFont_AddGlyph(ImFont* self,ImWchar c,float x0,float y0,float x1,float y1,float u0,float v0,float u1,float v1,float advance_x);
void ImFont_AddRemapChar(ImFont* self,ImWchar dst,ImWchar src,bool overwrite_dst);
void ImFont_SetFallbackChar(ImFont* self,ImWchar c);


/////////////////////////hand written functions
//no LogTextV
void igLogText(const char *fmt, ...);
//no appendfV
void ImGuiTextBuffer_appendf(struct ImGuiTextBuffer *buffer, const char *fmt, ...);
//for getting FLT_MAX in bindings
float igGET_FLT_MAX();
//not const args from & to *
void igColorConvertRGBtoHSV(float r,float g,float b,float *out_h,float *out_s,float *out_v);
void igColorConvertHSVtoRGB(float h,float s,float v,float *out_r,float *out_g,float *out_b);

PIM_C_END

#endif // CIMGUI_INCLUDED
