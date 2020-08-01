/********************************************************************/
/*                                                                  */
/*  pcs_rtl.c     Platform idependent process handling functions.   */
/*  Copyright (C) 1989 - 2014  Thomas Mertes                        */
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
/*  File: seed7/src/pcs_rtl.c                                       */
/*  Changes: 2014  Thomas Mertes                                    */
/*  Content: Platform idependent process handling functions.        */
/*                                                                  */
/********************************************************************/

#define LOG_FUNCTIONS 0
#define VERBOSE_EXCEPTIONS 0

#include "version.h"

#include "stdio.h"

#include "common.h"
#include "data_rtl.h"
#include "pcs_drv.h"



/**
 *  Reinterpret the generic parameters as processType and call pcsCmp.
 *  Function pointers in C programs generated by the Seed7 compiler
 *  may point to this function. This assures correct behaviour even
 *  when sizeof(genericType) != sizeof(processType).
 */
intType pcsCmpGeneric (const genericType value1, const genericType value2)

  { /* pcsCmpGeneric */
    return pcsCmp(((const_rtlObjectType *) &value1)->value.processValue,
                  ((const_rtlObjectType *) &value2)->value.processValue);
  } /* pcsCmpGeneric */



void pcsCpy (processType *const process_to, const processType process_from)

  { /* pcsCpy */
    logFunction(printf("pcsCpy(" FMT_U_MEM " (usage=" FMT_U "), "
                       FMT_U_MEM " (usage=" FMT_U "))\n",
                       (memSizeType) *process_to,
                       *process_to != NULL ? (*process_to)->usage_count : (uintType) 0,
                       (memSizeType) process_from,
                       process_from != NULL ? process_from->usage_count : (uintType) 0););
    if (process_from != NULL) {
      process_from->usage_count++;
    } /* if */
    if (*process_to != NULL) {
      (*process_to)->usage_count--;
      if ((*process_to)->usage_count == 0) {
        pcsFree(*process_to);
      } /* if */
    } /* if */
    *process_to = process_from;
    logFunction(printf("pcsCpy(" FMT_U_MEM " (usage=" FMT_U "), "
                       FMT_U_MEM " (usage=" FMT_U ")) -->\n",
                       (memSizeType) *process_to,
                       *process_to != NULL ? (*process_to)->usage_count : (uintType) 0,
                       (memSizeType) process_from,
                       process_from != NULL ? process_from->usage_count : (uintType) 0););
  } /* pcsCpy */



void pcsCpyGeneric (genericType *const dest, const genericType source)

  { /* pcsCpyGeneric */
    pcsCpy(&((rtlObjectType *) dest)->value.processValue,
           ((const_rtlObjectType *) &source)->value.processValue);
  } /* pcsCpyGeneric */



processType pcsCreate (const processType process_from)

  { /* pcsCreate */
    logFunction(printf("pcsCreate(" FMT_U_MEM ") (usage=" FMT_U ")\n",
                       (memSizeType) process_from,
                       process_from != NULL ? process_from->usage_count : (uintType) 0););
    if (process_from != NULL) {
      process_from->usage_count++;
    } /* if */
    logFunction(printf("pcsCreate --> " FMT_U_MEM " (usage=" FMT_U ")\n",
                       (memSizeType) process_from,
                       process_from != NULL ? process_from->usage_count : (uintType) 0););
    return process_from;
  } /* pcsCreate */



/**
 *  Generic Create function to be used via function pointers.
 *  Function pointers in C programs generated by the Seed7 compiler
 *  may point to this function. This assures correct behaviour even
 *  when sizeof(genericType) != sizeof(processType).
 */
genericType pcsCreateGeneric (const genericType from_value)

  {
    rtlObjectType result;

  /* pcsCreateGeneric */
    INIT_GENERIC_PTR(result.value.genericValue);
    result.value.processValue =
        pcsCreate(((const_rtlObjectType *) &from_value)->value.processValue);
    return result.value.genericValue;
  } /* pcsCreateGeneric */



void pcsDestr (const processType old_process)

  { /* pcsDestr */
    logFunction(printf("pcsDestr(" FMT_U_MEM ") (usage=" FMT_U ")\n",
                       (memSizeType) old_process,
                       old_process != NULL ? old_process->usage_count : (uintType) 0););
    if (old_process != NULL) {
      old_process->usage_count--;
      if (old_process->usage_count == 0) {
        pcsFree(old_process);
      } /* if */
    } /* if */
  } /* pcsDestr */



/**
 *  Generic Destr function to be used via function pointers.
 *  Function pointers in C programs generated by the Seed7 compiler
 *  may point to this function. This assures correct behaviour even
 *  when sizeof(genericType) != sizeof(processType).
 */
void pcsDestrGeneric (const genericType old_value)

  { /* pcsDestrGeneric */
    pcsDestr(((const_rtlObjectType *) &old_value)->value.processValue);
  } /* pcsDestrGeneric */
