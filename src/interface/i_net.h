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

qsocket_t* NET_NewQSocket(void);
void NET_FreeQSocket(qsocket_t* sock);
double SetNetTime(void);
void NET_Init(void);
void NET_Shutdown(void);
// returns a new connection number if there is one pending, else -1
qsocket_t* NET_CheckNewConnections(void);
// called by client to connect to a host.  Returns -1 if not able to
qsocket_t* NET_Connect(char *host);
// Returns true or false if the given qsocket can currently accept a
// message to be transmitted.
qboolean NET_CanSendMessage(qsocket_t *sock);
// returns data in net_message sizebuf
// returns 0 if no data is waiting
// returns 1 if a message was received
// returns 2 if an unreliable message was received
// returns -1 if the connection died
i32 NET_GetMessage(qsocket_t *sock);
// returns 0 if the message connot be delivered reliably, but the connection
//  is still considered valid
// returns 1 if the message was sent properly
// returns -1 if the connection died
i32 NET_SendMessage(qsocket_t *sock, sizebuf_t *data);
i32 NET_SendUnreliableMessage(qsocket_t *sock, sizebuf_t *data);
// This is a reliable *blocking* send to all attached clients.
i32 NET_SendToAll(sizebuf_t *data, i32 blocktime);
// if a dead connection is returned by a get or send function, this function
// should be called when it is convenient
void NET_Close(qsocket_t *sock);
// Server calls when a client is kicked off for a game related misbehavior
// like an illegal protocal conversation.  Client calls when disconnecting
// from a server.
// A netcon_t number will not be reused until this function is called for it
void NET_Poll(void);
void SchedulePollProcedure(PollProcedure *pp, double timeOffset);
void NET_Slist_f(void);

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
