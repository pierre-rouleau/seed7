/********************************************************************/
/*                                                                  */
/*  hi   Interpreter for Seed7 programs.                            */
/*  Copyright (C) 1990 - 2000  Thomas Mertes                        */
/*                                                                  */
/*  This program is free software; you can redistribute it and/or   */
/*  modify it under the terms of the GNU General Public License as  */
/*  published by the Free Software Foundation; either version 2 of  */
/*  the License, or (at your option) any later version.             */
/*                                                                  */
/*  This program is distributed in the hope that it will be useful, */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of  */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   */
/*  GNU General Public License for more details.                    */
/*                                                                  */
/*  You should have received a copy of the GNU General Public       */
/*  License along with this program; if not, write to the Free      */
/*  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,  */
/*  MA 02111-1307 USA                                               */
/*                                                                  */
/*  Module: Runtime                                                 */
/*  File: seed7/src/runfile.h                                       */
/*  Changes: 1994  Thomas Mertes                                    */
/*  Content: Copy, move and remove operations for files.            */
/*                                                                  */
/********************************************************************/

#ifdef ANSI_C

void remove_any_file (char *, errinfotype *);
void copy_any_file (char *, char *, errinfotype *);
void move_any_file (char *, char *, errinfotype *);

#else

void remove_any_file ();
void copy_any_file ();
void move_any_file ();

#endif
