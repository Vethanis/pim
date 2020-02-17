#pragma once

#include "common/guid.h"

template<typename T>
inline constexpr Guid TGuidOf() { SASSERT(false); return {}; }

#define DECL_TGUID(T) template<> inline constexpr Guid TGuidOf<T>() { return ToGuid(#T); }
