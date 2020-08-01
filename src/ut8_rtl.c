/********************************************************************/
/*                                                                  */
/*  ut8_rtl.c     Primitive actions for the UTF-8 file type.        */
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
/*  Free Software Foundation, Inc., 51 Franklin Street,             */
/*  Fifth Floor, Boston, MA  02110-1301, USA.                       */
/*                                                                  */
/*  Module: Seed7 Runtime Library                                   */
/*  File: seed7/src/ut8_rtl.c                                       */
/*  Changes: 2005  Thomas Mertes                                    */
/*  Content: Primitive actions for the UTF-8 file type.             */
/*                                                                  */
/********************************************************************/

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "sys/types.h"

#include "common.h"
#include "heaputl.h"
#include "striutl.h"
#include "fil_rtl.h"
#include "rtl_err.h"

#undef EXTERN
#define EXTERN
#include "ut8_rtl.h"


#define BUFFER_SIZE             2048
#define GETS_DEFAULT_SIZE    1048576
#define GETS_STRI_SIZE_DELTA    4096
#define READ_STRI_INIT_SIZE      256
#define READ_STRI_SIZE_DELTA    2048


typedef struct {
    memsizetype bytes_remaining;
    memsizetype bytes_missing;
    memsizetype chars_read;
    memsizetype chars_there;
  } read_state;



#ifdef ANSI_C

static INLINE void bytes_to_strelements (ustritype buffer, memsizetype bytes_in_buffer,
    strelemtype *stri_dest, read_state *state, errinfotype *err_info)
#else

static INLINE void bytes_to_strelements (buffer, bytes_in_buffer, stri_dest,
    state, err_info)
ustritype buffer;
memsizetype bytes_in_buffer;
strelemtype *stri_dest;
read_state *state;
errinfotype *err_info;
#endif

  { /* bytes_to_strelements */
    if (bytes_in_buffer != 0) {
      bytes_in_buffer += state->bytes_remaining;
      /* printf("#1# bytes_in_buffer=%d %X %X\n", bytes_in_buffer, buffer[0], buffer[1]); */
      state->bytes_remaining = utf8_to_stri(stri_dest, &state->chars_read, buffer, bytes_in_buffer);
      if (state->bytes_remaining != 0) {
        /* printf("#2# bytes_remaining=%d %X\n", state->bytes_remaining,
            buffer[bytes_in_buffer - state->bytes_remaining]); */
        state->bytes_missing = utf8_bytes_missing(&buffer[bytes_in_buffer - state->bytes_remaining],
                                                  state->bytes_remaining);
        /* printf("#3# bytes_missing=%d\n", state->bytes_missing); */
        if (state->bytes_missing != 0) {
          memmove(buffer, &buffer[bytes_in_buffer - state->bytes_remaining], state->bytes_remaining);
          /* printf("#4# %X %X\n", buffer[0], buffer[1]); */
          state->chars_there = 1;
        } else {
          /* printf("#5# bytes_in_buffer=%d bytes_remaining=%d bytes_missing=%d chars_requested=%d chars_missing=%d %X ftell=%ld\n",
              bytes_in_buffer, state->bytes_remaining, state->bytes_missing,
              chars_requested, chars_missing, buffer[bytes_in_buffer - state->bytes_remaining],
              ftell(aFile)); */
          *err_info = RANGE_ERROR;
          return;
        } /* if */
      } else {
        state->bytes_missing = 0;
        state->chars_there = 0;
      } /* if */
    } else {
      state->chars_read = 0;
    } /* if */
    /* printf("#6# chars_read=%d\n", state->chars_read); */
  } /* bytes_to_strelements */



#ifdef ANSI_C

static memsizetype read_utf8_string (filetype aFile, stritype stri, errinfotype *err_info)
#else

static memsizetype read_utf8_string (aFile, stri, err_info)
filetype aFile;
stritype stri;
errinfotype *err_info;
#endif

  {
    uchartype buffer[BUFFER_SIZE + 6];
    memsizetype bytes_in_buffer;
    memsizetype stri_pos;
    memsizetype chars_missing;
    read_state state = {0, 0, 1, 0};

  /* read_utf8_string */
    for (stri_pos = 0, chars_missing = stri->size;
        chars_missing >= BUFFER_SIZE - state.bytes_missing + state.chars_there &&
        (state.chars_read > 0 || state.chars_there) &&
        *err_info == OKAY_NO_ERROR;
        stri_pos += state.chars_read, chars_missing -= state.chars_read) {
      bytes_in_buffer = (memsizetype) fread(&buffer[state.bytes_remaining], 1,
          BUFFER_SIZE, aFile);
      if (bytes_in_buffer == 0 && stri_pos == 0 && ferror(aFile)) {
        *err_info = FILE_ERROR;
      } else {
        /* printf("#A# bytes_in_buffer=%d num_of_chars_read=%d\n",
            bytes_in_buffer, stri_pos); */
        bytes_to_strelements(buffer, bytes_in_buffer, &stri->mem[stri_pos],
             &state, err_info);
      } /* if */
    } /* for */
    for (; chars_missing > 0 && (state.chars_read > 0 || state.chars_there) &&
        *err_info == OKAY_NO_ERROR;
        stri_pos += state.chars_read, chars_missing -= state.chars_read) {
      bytes_in_buffer = (memsizetype) fread(&buffer[state.bytes_remaining], 1,
          chars_missing - state.chars_there + state.bytes_missing, aFile);
      if (bytes_in_buffer == 0 && stri_pos == 0 && ferror(aFile)) {
        *err_info = FILE_ERROR;
      } else {
        /* printf("#B# bytes_in_buffer=%d chars_missing=%d chars_read=%d chars_there=%d bytes_missing=%d num_of_chars_read=%d\n",
            bytes_in_buffer, chars_missing, chars_read, chars_there,
            state.bytes_missing, stri_pos); */
        bytes_to_strelements(buffer, bytes_in_buffer, &stri->mem[stri_pos],
             &state, err_info);
      } /* if */
    } /* for */
    return stri_pos;
  } /* read_utf8_string */



#ifdef ANSI_C

static stritype read_and_alloc_utf8_stri (filetype aFile, memsizetype chars_missing,
    memsizetype *num_of_chars_read, errinfotype *err_info)
#else

static stritype read_and_alloc_utf8_stri (aFile, chars_missing, num_of_chars_read, err_info)
filetype aFile;
memsizetype chars_missing;
memsizetype *num_of_chars_read;
errinfotype *err_info;
#endif

  {
    uchartype buffer[BUFFER_SIZE + 6];
    memsizetype bytes_in_buffer;
    memsizetype result_pos;
    memsizetype new_size;
    stritype resized_result;
    read_state state = {0, 0, 1, 0};
    stritype result;

  /* read_and_alloc_utf8_stri */
    /* printf("read_and_alloc_utf8_stri(%d, %d, *, *)\n", fileno(aFile), chars_missing); */
    if (!ALLOC_STRI_SIZE_OK(result, GETS_STRI_SIZE_DELTA)) {
      *err_info = MEMORY_ERROR;
      result = NULL;
    } else {
      result->size = GETS_STRI_SIZE_DELTA;
      for (result_pos = 0;
          chars_missing >= BUFFER_SIZE - state.bytes_missing + state.chars_there &&
          (state.chars_read > 0 || state.chars_there) &&
          *err_info == OKAY_NO_ERROR;
          result_pos += state.chars_read, chars_missing -= state.chars_read) {
        bytes_in_buffer = (memsizetype) fread(&buffer[state.bytes_remaining], 1,
            BUFFER_SIZE, aFile);
        if (bytes_in_buffer == 0 && result_pos == 0 && ferror(aFile)) {
          *err_info = FILE_ERROR;
        } else {
          /* printf("#A# bytes_in_buffer=%d num_of_chars_read=%d\n",
              bytes_in_buffer, result_pos); */
          if (result_pos + bytes_in_buffer > result->size) {
            new_size = result->size + GETS_STRI_SIZE_DELTA;
            REALLOC_STRI_CHECK_SIZE(resized_result, result, result->size, new_size);
            if (resized_result == NULL) {
              *err_info = MEMORY_ERROR;
              return result;
            } else {
              result = resized_result;
              COUNT3_STRI(result->size, new_size);
              result->size = new_size;
            } /* if */
          } /* if */
          bytes_to_strelements(buffer, bytes_in_buffer, &result->mem[result_pos],
              &state, err_info);
        } /* if */
      } /* for */
      for (; chars_missing > 0 && (state.chars_read > 0 || state.chars_there) &&
          *err_info == OKAY_NO_ERROR;
          result_pos += state.chars_read, chars_missing -= state.chars_read) {
        bytes_in_buffer = (memsizetype) fread(&buffer[state.bytes_remaining], 1,
            chars_missing - state.chars_there + state.bytes_missing, aFile);
        if (bytes_in_buffer == 0 && result_pos == 0 && ferror(aFile)) {
          *err_info = FILE_ERROR;
        } else {
          /* printf("#B# bytes_in_buffer=%d chars_missing=%d chars_read=%d chars_there=%d bytes_missing=%d num_of_chars_read=%d\n",
              bytes_in_buffer, chars_missing, chars_read, chars_there,
              state.bytes_missing, result_pos); */
          if (result_pos + bytes_in_buffer > result->size) {
            new_size = result->size + GETS_STRI_SIZE_DELTA;
            REALLOC_STRI_CHECK_SIZE(resized_result, result, result->size, new_size);
            if (resized_result == NULL) {
              *err_info = MEMORY_ERROR;
              return result;
            } else {
              result = resized_result;
              COUNT3_STRI(result->size, new_size);
              result->size = new_size;
            } /* if */
          } /* if */
          bytes_to_strelements(buffer, bytes_in_buffer, &result->mem[result_pos],
              &state, err_info);
        } /* if */
      } /* for */
      *num_of_chars_read = result_pos;
    } /* if */
    return result;
  } /* read_and_alloc_utf8_stri */



#ifdef ANSI_C

chartype ut8Getc (filetype aFile)
#else

chartype ut8Getc (aFile)
filetype aFile;
#endif

  {
    int character;
    chartype result;

  /* ut8Getc */
    character = getc(aFile);
    if (character != EOF && character > 0x7F) {
      /* character range 0x80 to 0xFF (128 to 255) */
      if ((character & 0xE0) == 0xC0) {
        /* character range 0xC0 to 0xDF (192 to 223) */
        result = (chartype) (character & 0x1F) << 6;
        character = getc(aFile);
        if ((character & 0xC0) == 0x80) {
          result |= character & 0x3F;
          if (result <= 0x7F) {
            raise_error(RANGE_ERROR);
            return 0;
          } /* if */
        } else {
          raise_error(RANGE_ERROR);
          return 0;
        } /* if */
      } else if ((character & 0xF0) == 0xE0) {
        /* character range 0xE0 to 0xEF (224 to 239) */
        result = (chartype) (character & 0x0F) << 12;
        character = getc(aFile);
        if ((character & 0xC0) == 0x80) {
          result |= (chartype) (character & 0x3F) << 6;
          character = getc(aFile);
          if ((character & 0xC0) == 0x80) {
            result |= character & 0x3F;
            if (result <= 0x7FF) {  /* (result >= 0xD800 && result <= 0xDFFF)) */
              raise_error(RANGE_ERROR);
              return 0;
            } /* if */
          } else {
            raise_error(RANGE_ERROR);
            return 0;
          } /* if */
        } else {
          raise_error(RANGE_ERROR);
          return 0;
        } /* if */
      } else if ((character & 0xF8) == 0xF0) {
        /* character range 0xF0 to 0xF7 (240 to 247) */
        result = (chartype) (character & 0x07) << 18;
        character = getc(aFile);
        if ((character & 0xC0) == 0x80) {
          result |= (chartype) (character & 0x3F) << 12;
          character = getc(aFile);
          if ((character & 0xC0) == 0x80) {
            result |= (chartype) (character & 0x3F) << 6;
            character = getc(aFile);
            if ((character & 0xC0) == 0x80) {
              result |= character & 0x3F;
              if (result <= 0xFFFF) {
                raise_error(RANGE_ERROR);
                return 0;
              } /* if */
            } else {
              raise_error(RANGE_ERROR);
              return 0;
            } /* if */
          } else {
            raise_error(RANGE_ERROR);
            return 0;
          } /* if */
        } else {
          raise_error(RANGE_ERROR);
          return 0;
        } /* if */
      } else if ((character & 0xFC) == 0xF8) {
        /* character range 0xF8 to 0xFB (248 to 251) */
        result = (chartype) (character & 0x03) << 24;
        character = getc(aFile);
        if ((character & 0xC0) == 0x80) {
          result |= (chartype) (character & 0x3F) << 18;
          character = getc(aFile);
          if ((character & 0xC0) == 0x80) {
            result |= (chartype) (character & 0x3F) << 12;
            character = getc(aFile);
            if ((character & 0xC0) == 0x80) {
              result |= (chartype) (character & 0x3F) << 6;
              character = getc(aFile);
              if ((character & 0xC0) == 0x80) {
                result |= character & 0x3F;
                if (result <= 0x1FFFFF) {
                  raise_error(RANGE_ERROR);
                  return 0;
                } /* if */
              } else {
                raise_error(RANGE_ERROR);
                return 0;
              } /* if */
            } else {
              raise_error(RANGE_ERROR);
              return 0;
            } /* if */
          } else {
            raise_error(RANGE_ERROR);
            return 0;
          } /* if */
        } else {
          raise_error(RANGE_ERROR);
          return 0;
        } /* if */
      } else if ((character & 0xFC) == 0xFC) {
        /* character range 0xFC to 0xFF (252 to 255) */
        result = (chartype) (character & 0x03) << 30;
        character = getc(aFile);
        if ((character & 0xC0) == 0x80) {
          result |= (chartype) (character & 0x3F) << 24;
          character = getc(aFile);
          if ((character & 0xC0) == 0x80) {
            result |= (chartype) (character & 0x3F) << 18;
            character = getc(aFile);
            if ((character & 0xC0) == 0x80) {
              result |= (chartype) (character & 0x3F) << 12;
              character = getc(aFile);
              if ((character & 0xC0) == 0x80) {
                result |= (chartype) (character & 0x3F) <<  6;
                character = getc(aFile);
                if ((character & 0xC0) == 0x80) {
                  result |= character & 0x3F;
                  if (result <= 0x3FFFFFF) {
                    raise_error(RANGE_ERROR);
                    return 0;
                  } /* if */
                } else {
                  raise_error(RANGE_ERROR);
                  return 0;
                } /* if */
              } else {
                raise_error(RANGE_ERROR);
                return 0;
              } /* if */
            } else {
              raise_error(RANGE_ERROR);
              return 0;
            } /* if */
          } else {
            raise_error(RANGE_ERROR);
            return 0;
          } /* if */
        } else {
          raise_error(RANGE_ERROR);
          return 0;
        } /* if */
      } else {
        /* character not in range 0xC0 to 0xFF (192 to 255) */
        raise_error(RANGE_ERROR);
        return 0;
      } /* if */
    } else {
      result = (chartype) character;
    } /* if */
    return result;
  } /* ut8Getc */



/**
 *  Return a string with 'length' characters read from an UTF-8 file.
 *  In order to work reasonable good for the common case (reading
 *  just some characters) memory for 'length' characters is requested
 *  with malloc(). After the data is read the result string is
 *  shrinked to the actual size (with realloc()). When 'length' is
 *  larger than GETS_DEFAULT_SIZE or the memory cannot be requested
 *  a different strategy is used. In this case the function trys to
 *  find out the number of available characters (this is possible
 *  for a regular file but not for a pipe). If this fails a third
 *  strategy is used. In this case a smaller block is requested. This
 *  block is filled with data, resized and filled in a loop.
 */
#ifdef ANSI_C

stritype ut8Gets (filetype aFile, inttype length)
#else

stritype ut8Gets (aFile, length)
filetype aFile;
inttype length;
#endif

  {
    memsizetype chars_requested;
    memsizetype bytes_there;
    memsizetype allocated_size;
    errinfotype err_info = OKAY_NO_ERROR;
    memsizetype num_of_chars_read;
    stritype resized_result;
    stritype result;

  /* ut8Gets */
    /* printf("ut8Gets(%d, %d)\n", fileno(aFile), length); */
    if (length < 0) {
      raise_error(RANGE_ERROR);
      result = NULL;
    } else {
      if ((uinttype) length > MAX_MEMSIZETYPE) {
        chars_requested = MAX_MEMSIZETYPE;
      } else {
        chars_requested = (memsizetype) length;
      } /* if */
      if (chars_requested > GETS_DEFAULT_SIZE) {
        /* Avoid requesting too much */
        result = NULL;
      } else {
        allocated_size = chars_requested;
        ALLOC_STRI_SIZE_OK(result, allocated_size);
      } /* if */
      if (result == NULL) {
        bytes_there = remainingBytesInFile(aFile);
        /* printf("bytes_there=%lu\n", bytes_there); */
        if (bytes_there != 0) {
          /* Now we know that bytes_there bytes are available in aFile */
          if (chars_requested <= bytes_there) {
            allocated_size = chars_requested;
          } else {
            allocated_size = bytes_there;
          } /* if */
          /* printf("allocated_size=%lu\n", allocated_size); */
          if (!ALLOC_STRI_CHECK_SIZE(result, allocated_size)) {
            /* printf("MAX_STRI_LEN=%lu, SIZ_STRI(MAX_STRI_LEN)=%lu\n",
                MAX_STRI_LEN, SIZ_STRI(MAX_STRI_LEN)); */
            raise_error(MEMORY_ERROR);
            return NULL;
          } /* if */
        } /* if */
      } /* if */
      if (result != NULL) {
        /* We have allocated a buffer for the requested number of chars
           or for the number of bytes which are available in the file */
        result->size = allocated_size;
        num_of_chars_read = read_utf8_string(aFile, result, &err_info);
      } else {
        /* We do not know how many bytes are avaliable therefore
           result is resized with GETS_STRI_SIZE_DELTA until we
           have read enough or we reach EOF */
        result = read_and_alloc_utf8_stri(aFile, chars_requested, &num_of_chars_read, &err_info);
      } /* if */
      if (err_info != OKAY_NO_ERROR) {
        if (result != NULL) {
          FREE_STRI(result, result->size);
        } /* if */
        raise_error(err_info);
        result = NULL;
      } else if (num_of_chars_read < result->size) {
        REALLOC_STRI_SIZE_OK(resized_result, result, result->size, num_of_chars_read);
        if (resized_result == NULL) {
          FREE_STRI(result, result->size);
          raise_error(MEMORY_ERROR);
          result = NULL;
        } else {
          result = resized_result;
          COUNT3_STRI(result->size, num_of_chars_read);
          result->size = num_of_chars_read;
        } /* if */
      } /* if */
    } /* if */
    /* printf("ut8Gets(%d, %d) ==> ", fileno(aFile), length);
        prot_stri(result);
        printf("\n"); */
    return result;
  } /* ut8Gets */



#ifdef ANSI_C

stritype ut8LineRead (filetype aFile, chartype *termination_char)
#else

stritype ut8LineRead (aFile, termination_char)
filetype aFile;
chartype *termination_char;
#endif

  {
    register int ch;
    register memsizetype position;
    uchartype *memory;
    memsizetype memlength;
    memsizetype newmemlength;
    bstritype resized_buffer;
    bstritype buffer;
    memsizetype result_size;
    stritype resized_result;
    stritype result;

  /* ut8LineRead */
    memlength = READ_STRI_INIT_SIZE;
    if (!ALLOC_BSTRI_SIZE_OK(buffer, memlength)) {
      raise_error(MEMORY_ERROR);
      result = NULL;
    } else {
      memory = buffer->mem;
      position = 0;
      while ((ch = getc(aFile)) != (int) '\n' && ch != EOF) {
        if (position >= memlength) {
          newmemlength = memlength + READ_STRI_SIZE_DELTA;
          REALLOC_BSTRI_CHECK_SIZE(resized_buffer, buffer, memlength, newmemlength);
          if (resized_buffer == NULL) {
            FREE_BSTRI(buffer, memlength);
            raise_error(MEMORY_ERROR);
            return NULL;
          } /* if */
          buffer = resized_buffer;
          COUNT3_BSTRI(memlength, newmemlength);
          memory = buffer->mem;
          memlength = newmemlength;
        } /* if */
        memory[position++] = (uchartype) ch;
      } /* while */
      if (ch == (int) '\n' && position != 0 && memory[position - 1] == '\r') {
        position--;
      } /* if */
      if (ch == EOF && position == 0 && ferror(aFile)) {
        FREE_BSTRI(buffer, memlength);
        raise_error(FILE_ERROR);
        result = NULL;
      } else {
        if (!ALLOC_STRI_CHECK_SIZE(result, position)) {
          FREE_BSTRI(buffer, memlength);
          raise_error(MEMORY_ERROR);
        } else {
          if (utf8_to_stri(result->mem, &result_size, buffer->mem, position) != 0) {
            FREE_BSTRI(buffer, memlength);
            FREE_STRI(result, position);
            raise_error(RANGE_ERROR);
            result = NULL;
          } else {
            FREE_BSTRI(buffer, memlength);
            REALLOC_STRI_SIZE_OK(resized_result, result, position, result_size);
            if (resized_result == NULL) {
              FREE_STRI(result, position);
              raise_error(MEMORY_ERROR);
              result = NULL;
            } else {
              result = resized_result;
              COUNT3_STRI(position, result_size);
              result->size = result_size;
              *termination_char = (chartype) ch;
            } /* if */
          } /* if */
        } /* if */
      } /* if */
    } /* if */
    return result;
  } /* ut8LineRead */



#ifdef ANSI_C

void ut8Seek (filetype aFile, inttype file_position)
#else

void ut8Seek (aFile, file_position)
filetype aFile;
inttype file_position;
#endif

  {
    int ch;

  /* ut8Seek */
    if (file_position <= 0) {
      raise_error(RANGE_ERROR);
    } else if (offsetSeek(aFile, (os_off_t) (file_position - 1), SEEK_SET) == 0) {
      while ((ch = getc(aFile)) != EOF &&
             (ch & 0xC0) == 0x80) ;
      if (ch != EOF) {
        if (offsetSeek(aFile, -1, SEEK_CUR) != 0) {
          raise_error(FILE_ERROR);
        } /* if */
      } /* if */
    } else {
      raise_error(FILE_ERROR);
    } /* if */
  } /* ut8Seek */



#ifdef ANSI_C

stritype ut8WordRead (filetype aFile, chartype *termination_char)
#else

stritype ut8WordRead (aFile, termination_char)
filetype aFile;
chartype *termination_char;
#endif

  {
    register int ch;
    register memsizetype position;
    uchartype *memory;
    memsizetype memlength;
    memsizetype newmemlength;
    bstritype resized_buffer;
    bstritype buffer;
    memsizetype result_size;
    stritype resized_result;
    stritype result;

  /* ut8WordRead */
    memlength = READ_STRI_INIT_SIZE;
    if (!ALLOC_BSTRI_SIZE_OK(buffer, memlength)) {
      raise_error(MEMORY_ERROR);
      result = NULL;
    } else {
      memory = buffer->mem;
      position = 0;
      do {
        ch = getc(aFile);
      } while (ch == (int) ' ' || ch == (int) '\t');
      while (ch != (int) ' ' && ch != (int) '\t' &&
          ch != (int) '\n' && ch != EOF) {
        if (position >= memlength) {
          newmemlength = memlength + READ_STRI_SIZE_DELTA;
          REALLOC_BSTRI_CHECK_SIZE(resized_buffer, buffer, memlength, newmemlength);
          if (resized_buffer == NULL) {
            FREE_BSTRI(buffer, memlength);
            raise_error(MEMORY_ERROR);
            return NULL;
          } /* if */
          buffer = resized_buffer;
          COUNT3_BSTRI(memlength, newmemlength);
          memory = buffer->mem;
          memlength = newmemlength;
        } /* if */
        memory[position++] = (uchartype) ch;
        ch = getc(aFile);
      } /* while */
      if (ch == (int) '\n' && position != 0 && memory[position - 1] == '\r') {
        position--;
      } /* if */
      if (ch == EOF && position == 0 && ferror(aFile)) {
        FREE_BSTRI(buffer, memlength);
        raise_error(FILE_ERROR);
        result = NULL;
      } else {
        if (!ALLOC_STRI_CHECK_SIZE(result, position)) {
          FREE_BSTRI(buffer, memlength);
          raise_error(MEMORY_ERROR);
        } else {
          if (utf8_to_stri(result->mem, &result_size, buffer->mem, position) != 0) {
            FREE_BSTRI(buffer, memlength);
            FREE_STRI(result, position);
            raise_error(RANGE_ERROR);
            result = NULL;
          } else {
            FREE_BSTRI(buffer, memlength);
            REALLOC_STRI_SIZE_OK(resized_result, result, position, result_size);
            if (resized_result == NULL) {
              FREE_STRI(result, position);
              raise_error(MEMORY_ERROR);
              result = NULL;
            } else {
              result = resized_result;
              COUNT3_STRI(position, result_size);
              result->size = result_size;
              *termination_char = (chartype) ch;
            } /* if */
          } /* if */
        } /* if */
      } /* if */
    } /* if */
    return result;
  } /* ut8WordRead */



#ifdef ANSI_C

void ut8Write (filetype aFile, const const_stritype stri)
#else

void ut8Write (aFile, stri)
filetype aFile;
stritype stri;
#endif

  {
    strelemtype *str;
    memsizetype len;
    memsizetype size;
    uchartype stri_buffer[MAX_UTF8_EXPANSION_FACTOR * 512];

  /* ut8Write */
#ifdef FWRITE_WRONG_FOR_READ_ONLY_FILES
    if (stri->size > 0 && (aFile->flags & _F_WRIT) == 0) {
      raise_error(FILE_ERROR);
      return;
    } /* if */
#endif
    for (str = stri->mem, len = stri->size; len >= 512; str += 512, len -= 512) {
      size = stri_to_utf8(stri_buffer, str, 512);
      if (size != fwrite(stri_buffer, sizeof(uchartype), (size_t) size, aFile)) {
        raise_error(FILE_ERROR);
        return;
      } /* if */
    } /* for */
    if (len > 0) {
      size = stri_to_utf8(stri_buffer, str, len);
      if (size != fwrite(stri_buffer, sizeof(uchartype), (size_t) size, aFile)) {
        raise_error(FILE_ERROR);
        return;
      } /* if */
    } /* if */
  } /* ut8Write */
