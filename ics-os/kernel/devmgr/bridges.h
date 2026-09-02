/*
  Name: bridges.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 11/01/04 18:04
  Description: This module brdiges calls between extensible modules. This module was
  largely influenced by the AOP (Aspect Oriented Programming) Joint-Point concept. It is
  now possible to effectively monitor inter-module communication by simply monitoring
  the bridges.The only disadvantage of bridges is that it will to some degree reduce
  system efficiency because of the added overhead, by how much I am still not certain.
  
    DEX educational extensible operating system 1.0 Beta
    Copyright (C) 2004  Joseph Emmanuel DL Dayo
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
*/

/******************* Function Prototypes Here****************************************/
PCB386 *bridges_ps_scheduler(PCB386 *prev);
PCB386 *bridges_ps_findprocess(int pid);
DWORD (*bridges_call)(devmgr_generic *dev, void **function, ...);
DWORD bridges_link(devmgr_generic *dev, void **function,
                DWORD p1,DWORD p2,DWORD p3,DWORD p4,DWORD p5,DWORD p6);

/* 64-bit variant for module functions that return values wider than a
   DWORD (e.g. total_blocks). Calling an int-returning function through
   the DWORD bridge, or a u64-returning one through the 64-bit bridge,
   are both well defined on SysV AMD64; mixing them is not (the caller
   would read only the low 32 bits of a 64-bit return, or the high 32
   bits of an int return are undefined). */
u64 (*bridges_call64)(devmgr_generic *dev, void **function, ...);
u64 bridges_link64(devmgr_generic *dev, void **function,
                 DWORD p1,DWORD p2,DWORD p3,DWORD p4,DWORD p5,DWORD p6);

