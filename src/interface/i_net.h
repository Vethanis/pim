#pragma once
/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net.h -- quake's interface to the networking layer

#include "interface/i_types.h"

typedef struct I_Net_s
{
    qsocket_t*(*_NET_NewQSocket)(void);
    void(*_NET_FreeQSocket)(qsocket_t* sock);
    double(*_SetNetTime)(void);
    void(*_NET_Init)(void);
    void(*_NET_Shutdown)(void);
    // returns a new connection number if there is one pending, else -1
    qsocket_t*(*_NET_CheckNewConnections)(void);
    // called by client to connect to a host.  Returns -1 if not able to
    qsocket_t*(*_NET_Connect)(char *host);
    // Returns true or false if the given qsocket can currently accept a
    // message to be transmitted.
    qboolean(*_NET_CanSendMessage)(qsocket_t *sock);
    // returns data in net_message sizebuf
    // returns 0 if no data is waiting
    // returns 1 if a message was received
    // returns 2 if an unreliable message was received
    // returns -1 if the connection died
    i32(*_NET_GetMessage)(qsocket_t *sock);
    // returns 0 if the message connot be delivered reliably, but the connection
    //  is still considered valid
    // returns 1 if the message was sent properly
    // returns -1 if the connection died
    i32(*_NET_SendMessage)(qsocket_t *sock, sizebuf_t *data);
    i32(*_NET_SendUnreliableMessage)(qsocket_t *sock, sizebuf_t *data);
    // This is a reliable *blocking* send to all attached clients.
    i32(*_NET_SendToAll)(sizebuf_t *data, i32 blocktime);
    // if a dead connection is returned by a get or send function, this function
    // should be called when it is convenient
    void(*_NET_Close)(qsocket_t *sock);
    // Server calls when a client is kicked off for a game related misbehavior
    // like an illegal protocal conversation.  Client calls when disconnecting
    // from a server.
    // A netcon_t number will not be reused until this function is called for it
    void(*_NET_Poll)(void);
    void(*_SchedulePollProcedure)(PollProcedure *pp, double timeOffset);
    void(*_NET_Slist_f)(void);
} I_Net_t;
extern I_Net_t I_Net;

#define NET_NewQSocket(...) I_Net._NET_NewQSocket(__VA_ARGS__)
#define NET_FreeQSocket(...) I_Net._NET_FreeQSocket(__VA_ARGS__)
#define SetNetTime(...) I_Net._SetNetTime(__VA_ARGS__)
#define NET_Init(...) I_Net._NET_Init(__VA_ARGS__)
#define NET_Shutdown(...) I_Net._NET_Shutdown(__VA_ARGS__)
#define NET_CheckNewConnections(...) I_Net._NET_CheckNewConnections(__VA_ARGS__)
#define NET_Connect(...) I_Net._NET_Connect(__VA_ARGS__)
#define NET_CanSendMessage(...) I_Net._NET_CanSendMessage(__VA_ARGS__)
#define NET_GetMessage(...) I_Net._NET_GetMessage(__VA_ARGS__)
#define NET_SendMessage(...) I_Net._NET_SendMessage(__VA_ARGS__)
#define NET_SendUnreliableMessage(...) I_Net._NET_SendUnreliableMessage(__VA_ARGS__)
#define NET_SendToAll(...) I_Net._NET_SendToAll(__VA_ARGS__)
#define NET_Close(...) I_Net._NET_Close(__VA_ARGS__)
#define NET_Poll(...) I_Net._NET_Poll(__VA_ARGS__)
#define SchedulePollProcedure(...) I_Net._SchedulePollProcedure(__VA_ARGS__)
#define NET_Slist_f(...) I_Net._NET_Slist_f(__VA_ARGS__)

// This is the network info/connection protocol.  It is used to find Quake
// servers, get info about them, and connect to them.  Once connected, the
// Quake game protocol (documented elsewhere) is used.
//
//
// General notes:
// game_name is currently always "QUAKE", but is there so this same protocol
//  can be used for future games as well; can you say Quake2?
//
// CCREQ_CONNECT
//  string game_name    "QUAKE"
//  byte net_protocol_version NET_PROTOCOL_VERSION
//
// CCREQ_SERVER_INFO
//  string game_name    "QUAKE"
//  byte net_protocol_version NET_PROTOCOL_VERSION
//
// CCREQ_PLAYER_INFO
//  byte player_number
//
// CCREQ_RULE_INFO
//  string rule
//
//
//
// CCREP_ACCEPT
//  long port
//
// CCREP_REJECT
//  string reason
//
// CCREP_SERVER_INFO
//  string server_address
//  string host_name
//  string level_name
//  byte current_players
//  byte max_players
//  byte protocol_version NET_PROTOCOL_VERSION
//
// CCREP_PLAYER_INFO
//  byte player_number
//  string name
//  long colors
//  long frags
//  long connect_time
//  string address
//
// CCREP_RULE_INFO
//  string rule
//  string value

// note:
//  There are two address forms used above.  The short form is just a
//  port number.  The address that goes along with the port is defined as
//  "whatever address you receive this reponse from".  This lets us use
//  the host OS to solve the problem of multiple host addresses (possibly
//  with no routing between them); the host will use the right address
//  when we reply to the inbound connection request.  The long from is
//  a full address and port in a string.  It is used for returning the
//  address of a server that is not running locally.
