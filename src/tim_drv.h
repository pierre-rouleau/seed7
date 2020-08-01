/********************************************************************/
/*                                                                  */
/*  tim_drv.h     Prototypes for time handling functions.           */
/*  Copyright (C) 1989 - 2006  Thomas Mertes                        */
/*                                                                  */
/*  This file is part of the Seed7 Runtime Library.                 */
/*                                                                  */
/*  The Seed7 Runtime Library is free software; you can             */
/*  redistribute it and/or modify it under the terms of the GNU     */
/*  Lesser General Public License as published by the Free Software */
/*  Foundation; either version 2.1 of the License, or (at your      */
/*  option) any later version.                                      */
/*                                                                  */
/*  The Seed7 Runtime Library is distributed in the hope that it    */
/*  will be useful, but WITHOUT ANY WARRANTY; without even the      */
/*  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR */
/*  PURPOSE.  See the GNU Lesser General Public License for more    */
/*  details.                                                        */
/*                                                                  */
/*  You should have received a copy of the GNU Lesser General       */
/*  Public License along with this program; if not, write to the    */
/*  Free Software Foundation, Inc., 59 Temple Place, Suite 330,     */
/*  Boston, MA 02111-1307 USA                                       */
/*                                                                  */
/*  Module: Seed7 Runtime Library                                   */
/*  File: seed7/src/tim_drv.h                                       */
/*  Changes: 1992, 1993, 1994  Thomas Mertes                        */
/*  Content: Prototypes for time handling functions.                */
/*                                                                  */
/********************************************************************/

#ifdef ANSI_C

void timNow (inttype *, inttype *, inttype *, inttype *,
	     inttype *, inttype *, inttype *, inttype *);
void get_time (time_t *, long *, long *);
void await_time (time_t, long);

#else

void timNow ();
void get_time ();
void await_time ();

#endif
