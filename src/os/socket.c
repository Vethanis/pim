#include "os/socket.h"
#include "common/profiler.h"
#include <string.h>

#if PLAT_WINDOWS

#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include <Windows.h>
#include <winsock.h>
#include <ctype.h>
#pragma comment(lib, "Ws2_32.lib")

static WSADATA s_wsadata;
static i32 s_hasInit;

static SOCKADDR_IN ToSockAddr(u32 addr, u16 port)
{
    ASSERT(addr != INADDR_NONE);
    ASSERT(port);

    // https://docs.microsoft.com/en-us/windows/win32/winsock/sockaddr-2
    SOCKADDR_IN name = { 0 };
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = addr;
    // https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-htons
    name.sin_port = htons(port);

    return name;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup
static void wsock_init(void)
{
    ASSERT(s_hasInit == 0);
    i32 rval = WSAStartup(MAKEWORD(1, 1), &s_wsadata);
    ASSERT(rval != SOCKET_ERROR);
    s_hasInit++;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsacleanup
static void wsock_shutdown(void)
{
    ASSERT(s_hasInit == 1);
    i32 rval = WSACleanup();
    ASSERT(rval != SOCKET_ERROR);
    --s_hasInit;
}

static bool wsock_url2addr(const char* url, u32* addr)
{
    if (!url)
    {
        return false;
    }
    u32 y = 0;
    if (isalpha(url[0]))
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-gethostbyname
        struct hostent *h = gethostbyname(url);
        ASSERT(h);
        char* addr0 = h->h_addr_list[0];
        y = *(u32*)addr0;
    }
    else
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-inet_addr
        y = inet_addr(url);
    }
    *addr = y;
    return y != INADDR_NONE;
}

static bool wsock_isopen(socket_t sock)
{
    uintptr_t hdl = (uintptr_t)sock.handle;
    return hdl != 0 && hdl != INVALID_SOCKET;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
static bool wsock_open(socket_t* sock, SocketProto proto)
{
    ASSERT(s_hasInit == 1);
    bool tcp = proto == SocketProto_TCP;
    SOCKET s = socket(
        AF_INET,
        tcp ? SOCK_STREAM : SOCK_DGRAM,
        tcp ? IPPROTO_TCP : IPPROTO_UDP);
    sock->handle = (void*)s;
    sock->proto = proto;
    return wsock_isopen(*sock);
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-closesocket
static void wsock_close(socket_t* sock)
{
    if (wsock_isopen(*sock))
    {
        i32 rval = closesocket((SOCKET)sock->handle);
        ASSERT(rval != SOCKET_ERROR);
    }
    sock->handle = NULL;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-bind
static bool wsock_bind(socket_t sock, u32 addr, u16 port)
{
    ASSERT(wsock_isopen(sock));
    if (wsock_isopen(sock))
    {
        SOCKADDR_IN name = ToSockAddr(addr, port);
        i32 rval = bind((SOCKET)sock.handle, (const struct sockaddr*)&name, sizeof(name));
        ASSERT(rval != SOCKET_ERROR);
        return rval != SOCKET_ERROR;
    }
    return false;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen
static bool wsock_listen(socket_t sock)
{
    ASSERT(wsock_isopen(sock));
    if (wsock_isopen(sock))
    {
        i32 rval = listen((SOCKET)sock.handle, SOMAXCONN);
        ASSERT(rval != SOCKET_ERROR);
        return rval != SOCKET_ERROR;
    }
    return false;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-accept
static socket_t wsock_accept(socket_t sock, u32* addr)
{
    ASSERT(addr);
    socket_t result;
    result.handle = (void*)INVALID_SOCKET;
    result.proto = sock.proto;
    *addr = 0;
    ASSERT(wsock_isopen(sock));
    if (wsock_isopen(sock))
    {
        SOCKADDR_IN saddr = { 0 };
        i32 len = sizeof(saddr);
        SOCKET s = accept((SOCKET)sock.handle, (struct sockaddr*)&saddr, &len);
        ASSERT(s != INVALID_SOCKET);
        if (s != INVALID_SOCKET)
        {
            *addr = saddr.sin_addr.s_addr;
            result.handle = (void*)s;
        }
    }
    return result;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-connect
static bool wsock_connect(socket_t sock, u32 addr, u16 port)
{
    ASSERT(wsock_isopen(sock));
    if (wsock_isopen(sock))
    {
        SOCKADDR_IN name = ToSockAddr(addr, port);
        i32 rval = connect((SOCKET)sock.handle, (const struct sockaddr*)&name, sizeof(name));
        return rval != SOCKET_ERROR;
    }
    return false;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-send
static i32 wsock_send(socket_t sock, const void* src, u32 len)
{
    i32 rval = -1;
    ASSERT(src);
    ASSERT(len);
    ASSERT(wsock_isopen(sock));
    if (wsock_isopen(sock))
    {
        rval = send((SOCKET)sock.handle, (const char*)src, len, 0x0);
        ASSERT(rval != SOCKET_ERROR);
    }
    return rval;
}

// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-recv
static i32 wsock_recv(socket_t sock, void* dst, u32 len)
{
    i32 rval = -1;
    ASSERT(dst);
    ASSERT(len);
    ASSERT(wsock_isopen(sock));
    if (wsock_isopen(sock))
    {
        rval = recv((SOCKET)sock.handle, (char*)dst, len, 0x0);
        ASSERT(rval != SOCKET_ERROR);
    }
    return rval;
}

#endif // PLAT_WINDOWS

void network_sys_init(void)
{
#if PLAT_WINDOWS
    wsock_init();
#endif // PLAT_WINDOWS
}

ProfileMark(pm_update, network_sys_update)
void network_sys_update(void)
{
    ProfileBegin(pm_update);

    ProfileEnd(pm_update);
}

void network_sys_shutdown(void)
{
#if PLAT_WINDOWS
    wsock_shutdown();
#endif // PLAT_WINDOWS
}

bool network_url2addr(const char* url, u32* addrOut)
{
#if PLAT_WINDOWS
    return wsock_url2addr(url, addrOut);
#endif // PLAT_WINDOWS
}

bool socket_open(socket_t* sock, SocketProto proto)
{
#if PLAT_WINDOWS
    return wsock_open(sock, proto);
#endif // PLAT_WINDOWS
}

void socket_close(socket_t* sock)
{
#if PLAT_WINDOWS
    wsock_close(sock);
#endif // PLAT_WINDOWS
}

bool socket_isopen(socket_t sock)
{
#if PLAT_WINDOWS
    return wsock_isopen(sock);
#endif // PLAT_WINDOWS
}

bool socket_bind(socket_t sock, u32 addr, u16 port)
{
#if PLAT_WINDOWS
    return wsock_bind(sock, addr, port);
#endif // PLAT_WINDOWS
}

bool socket_listen(socket_t sock)
{
#if PLAT_WINDOWS
    return wsock_listen(sock);
#endif // PLAT_WINDOWS
}

socket_t socket_accept(socket_t sock, u32* addr)
{
#if PLAT_WINDOWS
    return wsock_accept(sock, addr);
#endif // PLAT_WINDOWS
}

bool socket_connect(socket_t sock, u32 addr, u16 port)
{
#if PLAT_WINDOWS
    return wsock_connect(sock, addr, port);
#endif // PLAT_WINDOWS
}

i32 socket_send(socket_t sock, const void* src, i32 len)
{
#if PLAT_WINDOWS
    return wsock_send(sock, src, len);
#endif // PLAT_WINDOWS
}

i32 socket_recv(socket_t sock, void* dst, i32 len)
{
#if PLAT_WINDOWS
    return wsock_recv(sock, dst, len);
#endif // PLAT_WINDOWS
}
