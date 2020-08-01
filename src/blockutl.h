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
/*  Module: General                                                 */
/*  File: seed7/src/blockutl.h                                      */
/*  Changes: 1992, 1993, 1994  Thomas Mertes                        */
/*  Content: Procedures to maintain objects of type blocktype.      */
/*                                                                  */
/********************************************************************/

#ifdef ANSI_C

void free_block (blocktype);
blocktype new_block (loclisttype, locobjtype,
    loclisttype, objecttype);
void get_result_var (locobjtype, typetype, objecttype, errinfotype *);
void get_return_var (locobjtype, typetype, errinfotype *);
loclisttype get_param_list (listtype, errinfotype *);
loclisttype get_local_var_list (listtype, errinfotype *);

#else

void free_block ();
blocktype new_block ();
void get_result_var ();
lvoid get_return_var ();
oclisttype get_param_list ();
loclisttype get_local_var_list ();

#endif
