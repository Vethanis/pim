#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    SocketProto_UDP = 0,
    SocketProto_TCP,
    SocketProto_COUNT
} SocketProto;

typedef struct Socket_s
{
    void* handle;
    SocketProto proto;
} Socket;

void NetSys_Init(void);
void NetSys_Update(void);
void NetSys_Shutdown(void);

bool Net_UrlToAddr(const char* url, u32* addrOut);

bool Socket_Open(Socket* sock, SocketProto proto);
void Socket_Close(Socket* sock);

bool Socket_IsOpen(Socket sock);
bool Socket_Bind(Socket sock, u32 addr, u16 port);
bool Socket_Listen(Socket sock);
Socket Socket_Accept(Socket sock, u32* addr);
bool Socket_Connect(Socket sock, u32 addr, u16 port);

i32 Socket_Send(Socket sock, const void* src, i32 len);
i32 Socket_Recv(Socket sock, void* dst, i32 len);

PIM_C_END
