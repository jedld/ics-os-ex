/*******************************************************************
DEX32 API (Application Programmers Interface)
This is the code that manages system calls from user mode programs (Level 3)
currently applications make sys calls using interrupt 0x30h (User Interrupt Gate) 
although a user procedure call is in the works

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
********************************************************************/
#ifndef __dex32API_h_

#define __dex32API_h_

/**
 * Handles dex32 system calls, supports a maximum of
 * 5 parameters per call with a single DWORD return value
 * this function is called using interrupt 0x30 by user-mode
 * applications
*/
#define API_MAXSYSCALLS 0x100

#define API_REQUIRE_INTS 0x1
typedef struct _api_systemcall{
   DWORD access_check;
   int flags;
   void *function_ptr;
}api_systemcall;

/* Args must be pointer-width so user buffers survive on x86_64. */
#ifdef __x86_64__
typedef unsigned long api_arg_t;
#else
typedef DWORD api_arg_t;
#endif

//The system call table
api_systemcall api_syscalltable[API_MAXSYSCALLS];

int api_addsystemcall(DWORD function_number, void *function_ptr, 
                        DWORD access_check, DWORD flags);
void api_init();
int api_removesystemcall(DWORD function_number);
api_arg_t api_syscall(api_arg_t fxn, api_arg_t val, api_arg_t val2,
                   api_arg_t val3, api_arg_t val4, api_arg_t val5);
#ifdef __x86_64__
api_arg_t syscallentry64(api_arg_t sysno, api_arg_t a0, api_arg_t a1,
                         api_arg_t a2, api_arg_t a3, api_arg_t a4);
#endif

#endif
