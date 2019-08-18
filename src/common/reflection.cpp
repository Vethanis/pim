#include "common/reflection.h"

template<typename T>
RfItems RfStore<T>::ms_items;

Reflect(char)

Reflect(i8)
Reflect(i16)
Reflect(i32)
Reflect(i64)

Reflect(u8)
Reflect(u16)
Reflect(u32)
Reflect(u64)

Reflect(RfTest)
ReflectMember(RfTest, a)
ReflectMember(RfTest, b)
ReflectMember(RfTest, c)
ReflectMember(RfTest, d)
