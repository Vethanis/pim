#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    SocketProto_UDP = 0,
    SocketProto_TCP,
    SocketProto_COUNT
} SocketProto;

typedef struct socket_s
{
    void* handle;
    SocketProto proto;
} socket_t;

void network_sys_init(void);
void network_sys_update(void);
void network_sys_shutdown(void);

bool network_url2addr(const char* url, u32* addrOut);

bool socket_open(socket_t* sock, SocketProto proto);
void socket_close(socket_t* sock);

bool socket_isopen(socket_t sock);
bool socket_bind(socket_t sock, u32 addr, u16 port);
bool socket_listen(socket_t sock);
socket_t socket_accept(socket_t sock, u32* addr);
bool socket_connect(socket_t sock, u32 addr, u16 port);

i32 socket_send(socket_t sock, const void* src, i32 len);
i32 socket_recv(socket_t sock, void* dst, i32 len);

PIM_C_END
