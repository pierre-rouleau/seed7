/********************************************************************/
/*                                                                  */
/*  arr_rtl.h     Primitive actions for the array type.             */
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
/*  Free Software Foundation, Inc., 51 Franklin Street,             */
/*  Fifth Floor, Boston, MA  02110-1301, USA.                       */
/*                                                                  */
/*  Module: Seed7 Runtime Library                                   */
/*  File: seed7/src/arr_rtl.h                                       */
/*  Changes: 1991, 1992, 1993, 1994, 2005, 2006  Thomas Mertes      */
/*  Content: Primitive actions for the array type.                  */
/*                                                                  */
/********************************************************************/

void freeStringArray (rtlArrayType anArray);
#ifdef USE_WMAIN
rtlArrayType getArgv (const int argc, const utf16striType *const argv,
    striType *arg_0, striType *programName, striType *exePath);
#else
rtlArrayType getArgv (const int argc, const cstriType *const argv,
    striType *arg_0, striType *programName, striType *exePath);
#endif
striType examineSearchPath (const const_striType fileName);
void freeRtlStriArray (rtlArrayType work_array, intType used_max_position);
void arrAppend (rtlArrayType *const arr_variable, const rtlArrayType extension);
rtlArrayType arrArrlit2 (intType start_position, rtlArrayType arr1);
rtlArrayType arrBaselit (const genericType element);
rtlArrayType arrBaselit2 (intType start_position, const genericType element);
rtlArrayType arrCat (rtlArrayType arr1, const rtlArrayType arr2);
rtlArrayType arrExtend (rtlArrayType arr1, const genericType element);
void arrFree (rtlArrayType oldArray);
rtlArrayType arrGen (const genericType element1, const genericType element2);
rtlArrayType arrHead (const const_rtlArrayType arr1, intType stop);
rtlArrayType arrHeadTemp (rtlArrayType *arr_temp, intType stop);
genericType arrIdxTemp (rtlArrayType *arr_temp, intType pos);
void arrInsert (rtlArrayType *arr_to, intType position, genericType element);
void arrInsertArray (rtlArrayType *arr_to, intType position, rtlArrayType elements);
void arrInsertArrayTemp (rtlArrayType *arr_to, intType position, rtlArrayType elements);
rtlArrayType arrMalloc (intType minPosition, intType maxPosition);
void arrPush (rtlArrayType *const arr_variable, const genericType element);
rtlArrayType arrRange (const const_rtlArrayType arr1, intType start, intType stop);
rtlArrayType arrRangeTemp (rtlArrayType *arr_temp, intType start, intType stop);
rtlArrayType arrRealloc (rtlArrayType arr, memSizeType oldSize, memSizeType newSize);
genericType arrRemove (rtlArrayType *arr_to, intType position);
rtlArrayType arrRemoveArray (rtlArrayType *arr_to, intType position, intType length);
rtlArrayType arrSort (rtlArrayType arr1, compareType cmp_func);
rtlArrayType arrSortReverse (rtlArrayType arr1, compareType cmp_func);
rtlArrayType arrSubarr (const const_rtlArrayType arr1, intType start, intType len);
rtlArrayType arrSubarrTemp (rtlArrayType *arr_temp, intType start, intType len);
rtlArrayType arrTail (const const_rtlArrayType arr1, intType start);
rtlArrayType arrTailTemp (rtlArrayType *arr_temp, intType start);
