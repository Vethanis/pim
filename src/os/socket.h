#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>
#include <stdbool.h>

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

bool network_url2addr(const char* url, uint32_t* addrOut);

bool socket_open(socket_t* sock, SocketProto proto);
void socket_close(socket_t* sock);

bool socket_isopen(socket_t sock);
bool socket_bind(socket_t sock, uint32_t addr, uint16_t port);
bool socket_listen(socket_t sock);
socket_t socket_accept(socket_t sock, uint32_t* addr);
bool socket_connect(socket_t sock, uint32_t addr, uint16_t port);

int32_t socket_send(socket_t sock, const void* src, int32_t len);
int32_t socket_recv(socket_t sock, void* dst, int32_t len);

PIM_C_END
