#pragma once

#include "common/int_types.h"

struct Socket
{
    usize m_handle;
    bool m_isTcp;

    static void Init();
    static void Shutdown();

    static bool UrlToAddress(cstr url, u32& addr);

    static Socket Open(bool tcp);
    bool IsOpen() const;
    void Close();

    bool Bind(u32 addr, u16 port);
    bool Listen();
    Socket Accept(u32& addr);

    bool Connect(u32 addr, u16 port);

    i32 Send(const void* src, u32 len);
    i32 Recv(void* dst, u32 len);
};
