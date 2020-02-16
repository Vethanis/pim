#pragma once

#include "common/guid.h"

template<typename T>
inline constexpr Guid TGuidOf() { SASSERT(false); return {}; }

#define DECL_TGUID(T) template<> inline constexpr Guid TGuidOf<T>() { return ToGuid(#T); }

DECL_TGUID(Guid)
DECL_TGUID(u8)
DECL_TGUID(i8)
DECL_TGUID(u16)
DECL_TGUID(i16)
DECL_TGUID(u32)
DECL_TGUID(i32)
DECL_TGUID(u64)
DECL_TGUID(i64)
DECL_TGUID(usize)
DECL_TGUID(isize)
DECL_TGUID(void*)
