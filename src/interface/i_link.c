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

#include "interface/i_link.h"

#if QUAKE_IMPL

void ClearLink(link_t *l)
{
    l->prev = l->next = l;
}

void RemoveLink(link_t *l)
{
    l->next->prev = l->prev;
    l->prev->next = l->next;
}

void InsertLinkBefore(link_t *l, link_t *before)
{
    l->next = before;
    l->prev = before->prev;
    l->prev->next = l;
    l->next->prev = l;
}

void InsertLinkAfter(link_t *l, link_t *after)
{
    l->next = after->next;
    l->prev = after;
    l->prev->next = l;
    l->next->prev = l;
}

#endif // QUAKE_IMPL
