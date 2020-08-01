/********************************************************************/
/*                                                                  */
/*  str_rtl.h     Primitive actions for the string type.            */
/*  Copyright (C) 1989 - 2008  Thomas Mertes                        */
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
/*  Free Software Foundation, Inc., 51 Franklin Street,             */
/*  Fifth Floor, Boston, MA  02110-1301, USA.                       */
/*                                                                  */
/*  Module: Seed7 Runtime Library                                   */
/*  File: seed7/src/str_rtl.h                                       */
/*  Changes: 1991, 1992, 1993, 1994, 2005  Thomas Mertes            */
/*  Content: Primitive actions for the string type.                 */
/*                                                                  */
/********************************************************************/

#ifdef ANSI_C

stritype concat_path (const_stritype stri1, const_stritype stri2);
void strAppend (stritype *const stri_to, const_stritype stri_from);
void strAppendTemp (stritype *const stri_to, const stritype stri_from);
inttype strChIpos (const const_stritype main_stri, const chartype searched,
    const inttype from_index);
inttype strChPos (const const_stritype main_stri, const chartype searched);
/* rtlArraytype strChSplit (const const_stritype main_stri,
    const chartype delimiter); */
stritype strCLit (const const_stritype stri);
inttype strCompare (const const_stritype stri1, const const_stritype stri2);
stritype strConcat (const const_stritype stri1, const const_stritype stri2);
stritype strConcatTemp (stritype stri1, const const_stritype stri2);
void strCopy (stritype *const stri_to, const const_stritype stri_from);
stritype strCreate (const const_stritype stri_from);
void strDestr (const stritype old_string);
stritype strEmpty (void);
booltype strGe (const const_stritype stri1, const const_stritype stri2);
booltype strGt (const const_stritype stri1, const const_stritype stri2);
inttype strHashCode (const const_stritype stri);
#ifdef ALLOW_STRITYPE_SLICES
stritype strHeadSlice (const const_stritype stri, const inttype stop, stritype slice);
#else
stritype strHead (const const_stritype stri, const inttype stop);
#endif
stritype strHeadTemp (const stritype stri, const inttype stop);
inttype strIpos (const const_stritype main_stri, const const_stritype searched,
    const inttype from_index);
booltype strLe (const const_stritype stri1, const const_stritype stri2);
stritype strLit (const const_stritype stri);
stritype strLow (const const_stritype stri);
stritype strLowTemp (const stritype stri);
stritype strLpad (const const_stritype stri, const inttype pad_size);
stritype strLpadTemp (const stritype stri, const inttype pad_size);
stritype strLpad0 (const const_stritype stri, const inttype pad_size);
stritype strLpad0Temp (const stritype stri, const inttype pad_size);
booltype strLt (const const_stritype stri1, const const_stritype stri2);
stritype strLtrim (const const_stritype stri);
stritype strMult (const const_stritype stri, const inttype factor);
inttype strPos (const const_stritype main_stri, const const_stritype searched);
void strPush (stritype *const stri_to, const chartype char_from);
#ifdef ALLOW_STRITYPE_SLICES
stritype strRangeSlice (const const_stritype stri, inttype start, inttype stop, stritype slice);
#else
stritype strRange (const const_stritype stri, inttype start, inttype stop);
#endif
inttype strRChIpos (const const_stritype main_stri, const chartype searched,
    const inttype from_index);
inttype strRChPos (const const_stritype main_stri, const chartype searched);
stritype strRepl (const const_stritype main_stri,
    const const_stritype searched, const const_stritype replace);
stritype strRpad (const const_stritype stri, const inttype pad_size);
inttype strRpos (const const_stritype main_stri, const const_stritype searched);
stritype strRtrim (const const_stritype stri);
/* rtlArraytype strSplit (const const_stritype main_stri,
    const const_stritype delimiter); */
#ifdef ALLOW_STRITYPE_SLICES
stritype strSubstrSlice (const const_stritype stri, inttype start, inttype len, stritype slice);
#else
stritype strSubstr (const const_stritype stri, inttype start, inttype len);
#endif
#ifdef ALLOW_STRITYPE_SLICES
stritype strTailSlice (const const_stritype stri, inttype start, stritype slice);
#else
stritype strTail (const const_stritype stri, inttype start);
#endif
stritype strToUtf8 (const const_stritype stri);
stritype strTrim (const const_stritype stri);
stritype strUp (const const_stritype stri);
stritype strUpTemp (const stritype stri);
stritype strUtf8ToStri (const_stritype stri8);

#else

stritype concat_path ();
void strAppend ();
void strAppendTemp ();
void strChAppend ();
inttype strChIpos ();
inttype strChPos ();
/* rtlArraytype strChSplit (); */
stritype strCLit ();
inttype strCompare ();
stritype strConcat ();
stritype strConcatTemp ();
void strCopy ();
stritype strCreate ();
void strDestr ();
stritype strEmpty ();
booltype strGe ();
booltype strGt ();
inttype strHashCode ();
#ifdef ALLOW_STRITYPE_SLICES
stritype strHeadSlice ();
#else
stritype strHead ();
#endif
stritype strHeadTemp ();
inttype strIpos ();
booltype strLe ();
stritype strLit ();
stritype strLow ();
stritype strLowTemp ();
stritype strLpad ();
stritype strLpadTemp ();
stritype strLpad0 ();
stritype strLpad0Temp ();
booltype strLt ();
stritype strMult ();
inttype strPos ();
#ifdef ALLOW_STRITYPE_SLICES
stritype strRangeSlice ();
#else
stritype strRange ();
#endif
inttype strRChIpos ();
inttype strRChPos ();
stritype strRepl ();
stritype strRpad ();
inttype strRpos ();
stritype strRtrim ();
/* rtlArraytype strSplit (); */
#ifdef ALLOW_STRITYPE_SLICES
stritype strSubstrSlice ();
#else
stritype strSubstr ();
#endif
#ifdef ALLOW_STRITYPE_SLICES
stritype strTailSlice ();
#else
stritype strTail ();
#endif
stritype strToUtf8 ();
stritype strTrim ();
stritype strUp ();
stritype strUpTemp ();
stritype strUtf8ToStri ();

#endif
