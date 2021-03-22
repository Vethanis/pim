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

// cmd.h -- Command buffer and command execution

#include "interface/i_types.h"

typedef struct I_Cmd_s
{
    // allocates an initial text buffer that will grow as needed
    void (*_Cbuf_Init)(void);
    // as new commands are generated from the console or keybindings,
    // the text is added to the end of the command buffer.
    void(*_Cbuf_AddText)(const char *text);
    // when a command wants to issue other commands immediately, the text is
    // inserted at the beginning of the buffer, before any remaining unexecuted
    // commands.
    void(*_Cbuf_InsertText)(const char *text);
    // Pulls off \n terminated lines of text from the command buffer and sends
    // them through Cmd_ExecuteString.  Stops when the buffer is empty.
    // Normally called once per frame, but may be explicitly invoked.
    // Do not call inside a command function!
    void(*_Cbuf_Execute)(void);

    void(*_Cmd_Init)(void);
    // called by the init functions of other parts of the program to
    // register commands and functions to call for them.
    // The cmd_name is referenced later, so it should not be in temp memory
    void(*_Cmd_AddCommand)(const char* cmd_name, xcommand_t function);
    // used by the cvar code to check for cvar / command name overlap
    qboolean(*_Cmd_Exists)(const char* cmd_name);
    // attempts to match a partial command for automatic command line completion
    // returns NULL if nothing fits
    const char* (*_Cmd_CompleteCommand)(const char* partial);
    // The functions that execute commands get their parameters with these
    // functions. Cmd_Argv () will return an empty string, not a NULL
    // if arg > argc, so string operations are allways safe.
    i32(*_Cmd_Argc)(void);
    const char* (*_Cmd_Argv)(i32 arg);
    const char* (*_Cmd_Args)(void);
    // Returns the position (1 to argc-1) in the command's argument list
    // where the given parameter apears, or 0 if not present
    i32(*_Cmd_CheckParm)(const char* parm);
    // Takes a null terminated string.  Does not need to be /n terminated.
    // breaks the string up into arg tokens.
    void(*_Cmd_TokenizeString)(const char* text);
    // Parses a single line of text into arguments and tries to execute it.
    // The text can come from the command buffer, a remote client, or stdin.
    void(*_Cmd_ExecuteString)(const char* text, cmd_source_t src);
    // adds the current command line as a clc_stringcmd to the client message.
    // things like godmode, noclip, etc, are commands directed to the server,
    // so when they are typed in at the console, they will need to be forwarded.
    void(*_Cmd_ForwardToServer)(void);
    // used by command functions to send output to either the graphics console or
    // passed as a print message to the client
    void(*_Cmd_Print)(const char* text);
} I_Cmd_t;
extern I_Cmd_t I_Cmd;

#define Cbuf_Init(...) I_Cmd._Cbuf_Init(__VA_ARGS__)
#define Cbuf_AddText(...) I_Cmd._Cbuf_AddText(__VA_ARGS__)
#define Cbuf_InsertText(...) I_Cmd._Cbuf_InsertText(__VA_ARGS__)
#define Cbuf_Execute(...) I_Cmd._Cbuf_Execute(__VA_ARGS__)

#define Cmd_Init(...) I_Cmd._Cmd_Init(__VA_ARGS__)
#define Cmd_AddCommand(...) I_Cmd._Cmd_AddCommand(__VA_ARGS__)
#define Cmd_Exists(...) I_Cmd._Cmd_Exists(__VA_ARGS__)
#define Cmd_CompleteCommand(...) I_Cmd._Cmd_CompleteCommand(__VA_ARGS__)
#define Cmd_Argc(...) I_Cmd._Cmd_Argc(__VA_ARGS__)
#define Cmd_Argv(...) I_Cmd._Cmd_Argv(__VA_ARGS__)
#define Cmd_Args(...) I_Cmd._Cmd_Args(__VA_ARGS__)
#define Cmd_CheckParm(...) I_Cmd._Cmd_CheckParm(__VA_ARGS__)
#define Cmd_TokenizeString(...) I_Cmd._Cmd_TokenizeString(__VA_ARGS__)
#define Cmd_ExecuteString(...) I_Cmd._Cmd_ExecuteString(__VA_ARGS__)
#define Cmd_ForwardToServer(...) I_Cmd._Cmd_ForwardToServer(__VA_ARGS__)
#define Cmd_Print(...) I_Cmd._Cmd_Print(__VA_ARGS__)

/*

Any number of commands can be added in a frame, from several different sources.
Most commands come from either keybindings or console line input, but remote
servers can also send across commands and entire text files can be execed.

The + command line options are also added to the command buffer.

The game starts with a Cbuf_AddText ("exec quake.rc\n"); Cbuf_Execute ();

*/

/*

Command execution takes a null terminated string, breaks it into tokens,
then searches for a command or variable that matches the first token.

Commands can come from three sources, but the handler functions may choose
to dissallow the action or forward it to a remote server if the source is
not apropriate.

*/
