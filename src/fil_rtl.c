/********************************************************************/
/*                                                                  */
/*  fil_rtl.c     Primitive actions for the primitive file type.    */
/*  Copyright (C) 1989 - 2005  Thomas Mertes                        */
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
/*  File: seed7/src/fil_rtl.c                                       */
/*  Changes: 1992, 1993, 1994  Thomas Mertes                        */
/*  Content: Primitive actions for the primitive file type.         */
/*                                                                  */
/********************************************************************/

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "version.h"
#include "common.h"
#include "heaputl.h"
#include "striutl.h"
#include "rtl_err.h"

#undef EXTERN
#define EXTERN
#include "fil_rtl.h"


#define BUFFER_SIZE 2048



#ifdef ANSI_C

stritype filLineRead (filetype fil1, chartype *termination_char)
#else

stritype filLineRead (fil1, termination_char)
filetype fil1;
chartype *termination_char;
#endif

  {
    register int ch;
    register memsizetype position;
    strelemtype *memory;
    memsizetype memlength;
    memsizetype newmemlength;
    stritype result;

  /* filLineRead */
    memlength = 256;
    if (!ALLOC_STRI(result, memlength)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      COUNT_STRI(memlength);
      memory = result->mem;
      position = 0;
      while ((ch = getc(fil1)) != '\n' && ch != EOF) {
        if (position >= memlength) {
          newmemlength = memlength + 2048;
          if (!RESIZE_STRI(result, memlength, newmemlength)) {
            raise_error(MEMORY_ERROR);
            return(NULL);
          } /* if */
          COUNT3_STRI(memlength, newmemlength);
          memory = result->mem;
          memlength = newmemlength;
        } /* if */
        memory[position++] = (strelemtype) ch;
      } /* while */
      if (!RESIZE_STRI(result, memlength, position)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } /* if */
      COUNT3_STRI(memlength, position);
      result->size = position;
      *termination_char = (chartype) ch;
      return(result);
    } /* if */
  } /* filLineRead */



#ifdef ANSI_C

stritype filLit (filetype fil1)
#else

stritype filLit (fil1)
filetype fil1;
#endif

  {
    cstritype file_name;
    memsizetype length;
    stritype result;

  /* filLit */
    if (fil1 == NULL) {
      file_name = "NULL";
    } else if (fil1 == stdin) {
      file_name = "stdin";
    } else if (fil1 == stdout) {
      file_name = "stdout";
    } else if (fil1 == stderr) {
      file_name = "stderr";
    } else {
      file_name = "file";
    } /* if */
    length = strlen(file_name);
    if (!ALLOC_STRI(result, length)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      COUNT_STRI(length);
      result->size = length;
      stri_expand(result->mem, file_name, length);
      return(result);
    } /* if */
  } /* filLit */



#ifdef ANSI_C

inttype filLng (filetype fil1)
#else

inttype filLng (fil1)
filetype fil1;
#endif

  {
    long current_file_position;
    inttype file_length;

  /* filLng */
    current_file_position = ftell(fil1);
    fseek(fil1, 0, SEEK_END);
    file_length = (inttype) ftell(fil1);
    fseek(fil1, current_file_position, SEEK_SET);
    return(file_length);
  } /* filLng */



#ifdef ANSI_C

filetype filOpen (stritype str1, stritype str2)
#else

filetype filOpen (str1, str2)
stritype str1;
stritype str2;
#endif

  {
    cstritype file_name;
    cstritype file_mode;
    char mode[4];
    filetype result;

  /* filOpen */
    file_name = cp_to_cstri(str1);
    if (file_name == NULL) {
      raise_error(MEMORY_ERROR);
      result = NULL;
    } else {
      file_mode = cp_to_cstri(str2);
      if (file_mode == NULL) {
        raise_error(MEMORY_ERROR);
        result = NULL;
      } else {
        mode[0] = '\0';
        if (file_mode[0] == 'r' ||
            file_mode[0] == 'w' ||
            file_mode[0] == 'a') {
          if (file_mode[1] == '\0') {
            /* Binary mode
               r ... Open file for reading. 
               w ... Truncate to zero length or create file for writing. 
               a ... Append; open or create file for writing at end-of-file. 
            */
            mode[0] = file_mode[0];
            mode[1] = 'b';
            mode[2] = '\0';
          } else if (file_mode[1] == '+') {
            if (file_mode[2] == '\0') {
              /* Binary mode
                 r+ ... Open file for update (reading and writing). 
                 w+ ... Truncate to zero length or create file for update. 
                 a+ ... Append; open or create file for update, writing at end-of-file. 
              */
              mode[0] = file_mode[0];
              mode[1] = 'b';
              mode[2] = '+';
              mode[3] = '\0';
            } /* if */
          } else if (file_mode[1] == 't') {
            if (file_mode[2] == '\0') {
              /* Text mode
                 rt ... Open file for reading. 
                 wt ... Truncate to zero length or create file for writing. 
                 at ... Append; open or create file for writing at end-of-file. 
              */
              mode[0] = file_mode[0];
              mode[1] = '\0';
            } else if (file_mode[2] == '+') {
              /* Text mode
                 rt+ ... Open file for update (reading and writing). 
                 wt+ ... Truncate to zero length or create file for update. 
                 at+ ... Append; open or create file for update, writing at end-of-file. 
              */
              mode[0] = file_mode[0];
              mode[1] = '+';
              mode[2] = '\0';
            } /* if */
          } /* if */
        } /* if */
        if (mode[0] == '\0') {
          raise_error(RANGE_ERROR);
          result = NULL;
        } else {
          result = fopen(file_name, mode);
        } /* if */
        free_cstri(file_mode, str2);
      } /* if */
      free_cstri(file_name, str1);
    } /* if */
    return(result);
  } /* filOpen */



#ifdef ANSI_C

stritype filStriRead (filetype fil1, inttype length)
#else

stritype filStriRead (fil1, length)
filetype fil1;
inttype length;
#endif

  {
    long current_file_position;
    memsizetype bytes_there;
    memsizetype bytes_requested;
    memsizetype result_size;
    stritype result;

  /* filStriRead */
    if (length < 0) {
      raise_error(RANGE_ERROR);
      return(NULL);
    } else {
      bytes_requested = (memsizetype) length;
      if (!ALLOC_STRI(result, bytes_requested)) {
        if ((current_file_position = ftell(fil1)) != -1) {
          fseek(fil1, 0, SEEK_END);
          bytes_there = (memsizetype) (ftell(fil1) - current_file_position);
          fseek(fil1, current_file_position, SEEK_SET);
          if (bytes_there < bytes_requested) {
            bytes_requested = bytes_there;
            if (!ALLOC_STRI(result, bytes_requested)) {
              raise_error(MEMORY_ERROR);
              return(NULL);
            } /* if */
          } else {
            raise_error(MEMORY_ERROR);
            return(NULL);
          } /* if */
        } else {
          raise_error(MEMORY_ERROR);
          return(NULL);
        } /* if */
      } /* if */
      COUNT_STRI(bytes_requested);
      result_size = (memsizetype) fread(result->mem, 1,
          (SIZE_TYPE) bytes_requested, fil1);
#ifdef WIDE_CHAR_STRINGS
      if (result_size > 0) {
        uchartype *from = &((uchartype *) result->mem)[result_size - 1];
        strelemtype *to = &result->mem[result_size - 1];
        memsizetype number = result_size;

        for (; number > 0; from--, to--, number--) {
          *to = *from;
        } /* for */
      } /* if */
#endif
      result->size = result_size;
      if (result_size < bytes_requested) {
        if (!RESIZE_STRI(result, bytes_requested, result_size)) {
          raise_error(MEMORY_ERROR);
          return(NULL);
        } /* if */
        COUNT3_STRI(bytes_requested, result_size);
      } /* if */
    } /* if */
    return(result);
  } /* filStriRead */



#ifdef ANSI_C

stritype filWordRead (filetype fil1, chartype *termination_char)
#else

stritype filWordRead (fil1, termination_char)
filetype fil1;
chartype *termination_char;
#endif

  {
    register int ch;
    register memsizetype position;
    strelemtype *memory;
    memsizetype memlength;
    memsizetype newmemlength;
    stritype result;

  /* filWordRead */
    memlength = 256;
    if (!ALLOC_STRI(result, memlength)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      COUNT_STRI(memlength);
      memory = result->mem;
      position = 0;
      do {
        ch = getc(fil1);
      } while (ch == ' ' || ch == '\t');
      while (ch != ' ' && ch != '\t' &&
          ch != '\n' && ch != EOF) {
        if (position >= memlength) {
          newmemlength = memlength + 2048;
          if (!RESIZE_STRI(result, memlength, newmemlength)) {
            raise_error(MEMORY_ERROR);
            return(NULL);
          } /* if */
          COUNT3_STRI(memlength, newmemlength);
          memory = result->mem;
          memlength = newmemlength;
        } /* if */
        memory[position++] = (strelemtype) ch;
        ch = getc(fil1);
      } /* while */
      if (!RESIZE_STRI(result, memlength, position)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } /* if */
      COUNT3_STRI(memlength, position);
      result->size = position;
      *termination_char = (chartype) ch;
      return(result);
    } /* if */
  } /* filWordRead */



#ifdef ANSI_C

void filWrite (filetype fil1, stritype stri)
#else

void filWrite (stri)
filetype fil1;
stritype stri;
#endif

  { /* filWrite */
#ifdef WIDE_CHAR_STRINGS
    {
      register strelemtype *str;
      memsizetype len;
      register uchartype *ustri;
      register memsizetype buf_len;
      uchartype buffer[BUFFER_SIZE];

      for (str = stri->mem, len = stri->size;
          len >= BUFFER_SIZE; len -= BUFFER_SIZE) {
        for (ustri = buffer, buf_len = BUFFER_SIZE;
            buf_len > 0; str++, ustri++, buf_len--) {
          if (*str >= 256) {
            raise_error(RANGE_ERROR);
            return;
          } /* if */
          *ustri = (uchartype) *str;
        } /* for */
        fwrite(buffer, sizeof(uchartype), BUFFER_SIZE, fil1);
      } /* for */
      if (len > 0) {
        for (ustri = buffer, buf_len = len;
            buf_len > 0; str++, ustri++, buf_len--) {
          if (*str >= 256) {
            raise_error(RANGE_ERROR);
            return;
          } /* if */
          *ustri = (uchartype) *str;
        } /* for */
        fwrite(buffer, sizeof(uchartype), len, fil1);
      } /* if */
    }
#else
    fwrite(stri->mem, sizeof(strelemtype),
        (SIZE_TYPE) stri->size, fil1);
#endif
  } /* filWrite */
