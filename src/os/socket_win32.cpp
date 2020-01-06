#include "common/macro.h"

#if PLAT_WINDOWS

#include <Windows.h>
#include <winsock.h>
#include <ctype.h>
#pragma comment(lib, "ws2_32.lib")

#include "os/socket.h"
#include "common/macro.h"

static WSADATA s_wsadata;
static i32 s_hasInit;

static sockaddr_in ToSockAddr(u32 addr, u16 port)
{
    ASSERT(addr != INADDR_NONE);
    ASSERT(port);

    // https://docs.microsoft.com/en-us/windows/win32/winsock/sockaddr-2
    sockaddr_in name = {};
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = addr;
    name.sin_port = ::htons(port);

    return name;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup
void Socket::Init()
{
    ASSERT(s_hasInit == 0);
    i32 rval = ::WSAStartup(MAKEWORD(1, 1), &s_wsadata);
    ASSERT(rval != SOCKET_ERROR);
    s_hasInit++;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsacleanup
void Socket::Shutdown()
{
    ASSERT(s_hasInit == 1);
    i32 rval = ::WSACleanup();
    ASSERT(rval != SOCKET_ERROR);
    --s_hasInit;
}

bool Socket::UrlToAddress(cstr url, u32& addr)
{
    if (!url)
    {
        return false;
    }
    u32 y = 0;
    if (isalpha(url[0]))
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-gethostbyname
        hostent *h = ::gethostbyname(url);
        ASSERT(h);
        char* addr0 = h->h_addr_list[0];
        y = *(u32*)addr0;
    }
    else
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-inet_addr
        y = ::inet_addr(url);
    }
    addr = y;
    return y != INADDR_NONE;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
Socket Socket::Open(bool tcp)
{
    ASSERT(s_hasInit == 1);
    SOCKET sock = ::socket(
        AF_INET,
        tcp ? SOCK_STREAM : SOCK_DGRAM,
        tcp ? IPPROTO_TCP : IPPROTO_UDP);
    Socket s = {};
    s.m_handle = (usize)sock;
    s.m_isTcp = tcp;
    return s;
}

bool Socket::IsOpen() const
{
    return m_handle != 0 && m_handle != INVALID_SOCKET;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-closesocket
void Socket::Close()
{
    if (IsOpen())
    {
        i32 rval = ::closesocket((SOCKET)m_handle);
        ASSERT(rval != SOCKET_ERROR);
    }
    m_handle = 0;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-bind
bool Socket::Bind(u32 addr, u16 port)
{
    ASSERT(IsOpen());
    if (IsOpen())
    {
        sockaddr_in name = ToSockAddr(addr, port);
        i32 rval = ::bind((SOCKET)m_handle, (const sockaddr*)&name, sizeof(name));
        ASSERT(rval != SOCKET_ERROR);
        return rval != SOCKET_ERROR;
    }
    return false;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen
bool Socket::Listen()
{
    ASSERT(IsOpen());
    if (IsOpen())
    {
        i32 rval = ::listen((SOCKET)m_handle, SOMAXCONN);
        ASSERT(rval != SOCKET_ERROR);
        return rval != SOCKET_ERROR;
    }
    return false;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-accept
Socket Socket::Accept(u32& addr)
{
    ASSERT(IsOpen());
    if (IsOpen())
    {
        SOCKET sock = ::accept((SOCKET)m_handle, 0, 0);
        ASSERT(sock != INVALID_SOCKET);
        return { sock, m_isTcp };
    }
    return { INVALID_SOCKET, m_isTcp };
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-connect
bool Socket::Connect(u32 addr, u16 port)
{
    ASSERT(IsOpen());
    if (IsOpen())
    {
        sockaddr_in name = ToSockAddr(addr, port);
        i32 rval = ::connect((SOCKET)m_handle, (const sockaddr*)&name, sizeof(name));
        return rval != SOCKET_ERROR;
    }
    return false;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-send
i32 Socket::Send(const void* src, u32 len)
{
    ASSERT(src);
    ASSERT(len);
    ASSERT(IsOpen());
    i32 rval = -1;
    if (IsOpen())
    {
        rval = ::send((SOCKET)m_handle, (cstr)src, len, 0x0);
        ASSERT(rval != SOCKET_ERROR);
    }
    return rval;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-recv
i32 Socket::Recv(void* dst, u32 len)
{
    ASSERT(dst);
    ASSERT(len);
    ASSERT(IsOpen());
    i32 rval = -1;
    if (IsOpen())
    {
        rval = ::recv((SOCKET)m_handle, (char*)dst, len, 0x0);
        ASSERT(rval != SOCKET_ERROR);
    }
    return rval;
}

#endif // PLAT_WINDOWS
