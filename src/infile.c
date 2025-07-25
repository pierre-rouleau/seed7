/********************************************************************/
/*                                                                  */
/*  s7   Seed7 interpreter                                          */
/*  Copyright (C) 1990 - 2013, 2014, 2015, 2018  Thomas Mertes      */
/*                2021, 2023, 2025  Thomas Mertes                   */
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
/*  License along with this program; if not, write to the           */
/*  Free Software Foundation, Inc., 51 Franklin Street,             */
/*  Fifth Floor, Boston, MA  02110-1301, USA.                       */
/*                                                                  */
/*  Module: Analyzer - Infile                                       */
/*  File: seed7/src/infile.c                                        */
/*  Changes: 1990 - 1994, 2013, 2014, 2015, 2018  Thomas Mertes     */
/*           2021, 2023, 2025  Thomas Mertes                        */
/*  Content: Functions to open, close and read the source file.     */
/*                                                                  */
/********************************************************************/

#define LOG_FUNCTIONS 0
#define VERBOSE_EXCEPTIONS 0

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"

#if HAS_MMAP
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/mman.h"
#include "stat_drv.h"
#endif

#include "common.h"
#include "data.h"
#include "os_decls.h"
#include "heaputl.h"
#include "flistutl.h"
#include "striutl.h"
#include "info.h"
#include "stat.h"

#undef EXTERN
#define EXTERN
#define DO_INIT
#include "infile.h"


#define GET_INFILE_BUFFER TRUE
#define SIZE_IN_BUFFER 32768

static striType question_mark = NULL;



#if USE_ALTERNATE_NEXT_CHARACTER
int fill_buf (void)

  {
#if !HAS_MMAP
    memSizeType chars_read;
#endif
    int ch;

  /* fill_buf */
    logFunction(printf("fill_buf\n"););
#if HAS_MMAP
    ch = EOF;
#else
    if (in_file.fil != NULL &&
        (chars_read = fread(in_file.start, 1,
        (size_t) in_file.buffer_size, in_file.fil)) != 0) {
      in_file.nextch = in_file.start;
      in_file.beyond = in_file.start + chars_read;
      ch = next_character();
    } else {
      ch = EOF;
    } /* if */
#endif
    logFunction(printf("fill_buf --> %d\n", ch););
    return ch;
  } /* fill_buf */
#endif



static inline boolType speedup (void)

  {
    boolType okay;
#if HAS_MMAP
    int file_no;
    os_fstat_struct file_stat;
    memSizeType file_length;
#endif

  /* speedup */
    logFunction(printf("speedup\n"););
    okay = TRUE;
#if USE_ALTERNATE_NEXT_CHARACTER
#if HAS_MMAP
    file_no = os_fileno(in_file.fil);
    if (file_no != -1 && os_fstat(file_no, &file_stat) == 0) {
      if (file_stat.st_size >= 0 &&
          (unsigned_os_off_t) file_stat.st_size < MAX_MEMSIZETYPE) {
        file_length = (memSizeType) file_stat.st_size;
        if (unlikely(file_length == 0)) {
          logMessage(printf("speedup: The file size is 0.\n"););
          if (likely(ALLOC_UBYTES(in_file.start, 1))) {
            *in_file.start = '\0';
            in_file.nextch = in_file.start;
            in_file.beyond = in_file.start;
            in_file.buffer_size = 1;
          } else {
            okay = FALSE;
          } /* if */
        } else if ((in_file.start = (ustriType) mmap(NULL, file_length,
            PROT_READ, MAP_PRIVATE, file_no, 0)) != (ustriType) -1) {
          in_file.nextch = in_file.start;
          in_file.beyond = in_file.start + file_length;
          in_file.buffer_size = 0;
        } else {
          logError(printf("speedup: map(NULL, " FMT_U_MEM
                          ", PROT_READ, MAP_PRIVATE, %d, 0) failed:\n"
                          "errno=%d\nerror: %s\n",
                          file_length, file_no,
                          errno, strerror(errno)););
          if (likely(ALLOC_UBYTES(in_file.start, file_length))) {
            if (unlikely(fread(in_file.start, 1, file_length, in_file.fil) !=
                         file_length)) {
              logError(printf("speedup: fread(" FMT_X_MEM ", 1, "
                              FMT_U_MEM ", " FMT_X_MEM ") failed:\n"
                              "errno=%d\nerror: %s\n",
                              (memSizeType) in_file.start,
                              file_length, (memSizeType) in_file.fil,
                              errno, strerror(errno)););
              FREE_BYTES(in_file.start, file_length);
              okay = FALSE;
            } else {
              in_file.nextch = in_file.start;
              in_file.beyond = in_file.start + file_length;
              in_file.buffer_size = file_length;
              fseek(in_file.fil, 0, SEEK_SET);
            } /* if */
          } else {
            okay = FALSE;
          } /* if */
        } /* if */
      } else {
        okay = FALSE;
      } /* if */
    } else {
      okay = FALSE;
    } /* if */
#else
    if (GET_INFILE_BUFFER) {
      in_file.buffer_size = SIZE_IN_BUFFER;
    } else {
      in_file.buffer_size = 512;
    } /* if */
    if (likely(ALLOC_UBYTES(in_file.start, in_file.buffer_size))) {
      in_file.nextch = in_file.start + in_file.buffer_size;
      in_file.beyond = in_file.start;
    } else {
      in_file.buffer_size = 512;
      if (likely(ALLOC_UBYTES(in_file.start, in_file.buffer_size))) {
        in_file.nextch = in_file.start + in_file.buffer_size;
        in_file.beyond = in_file.start;
      } else {
        okay = FALSE;
      } /* if */
    } /* if */
#endif
#else
#if USE_INFILE_BUFFER
    if (GET_INFILE_BUFFER) {
      if (likely(ALLOC_UBYTES(in_file.buffer, SIZE_IN_BUFFER))) {
        setvbuf(in_file.fil, in_file.buffer, _IOFBF,
            (size_t) SIZE_IN_BUFFER);
      } /* if */
    } else {
      in_file.buffer = NULL;
    } /* if */
#endif
#endif
    logFunction(printf("speedup --> %d\n", okay););
    return okay;
  } /* speedup */



boolType openInfile (const_striType sourceFileName,
    striType absolutePath, fileNumType fileNumber, inFileType nextFile,
    boolType write_library_names, boolType write_line_numbers,
    errInfoType *err_info)

  {
    os_striType os_path;
    inFileType new_file;
    FILE *in_fil;
#if FOPEN_OPENS_DIRECTORIES
    int file_no;
    os_fstat_struct stat_buf;
#endif
    ustriType name_ustri;
    ustriType resized_name_ustri;
    memSizeType name_length;
    striType in_name;
    int path_info = PATH_IS_NORMAL;
    boolType isOpen = FALSE;

  /* openInfile */
    logFunction(printf("openInfile(\"%s\", %u, " FMT_U_MEM
                                   ", %d, %d, err_info=%d)\n",
                       striAsUnquotedCStri(sourceFileName),
                       fileNumber, (memSizeType) nextFile,
                       write_library_names, write_line_numbers,
                       *err_info););
    os_path = cp_to_os_path(sourceFileName, &path_info, err_info);
    if (likely(os_path != NULL)) {
      in_fil = os_fopen(os_path, os_mode_rb);
      if (unlikely(in_fil == NULL)) {
        logError(printf("openInfile: "
                        "fopen(\"" FMT_S_OS "\", \"" FMT_S_OS "\") failed:\n"
                        "errno=%d\nerror: %s\n",
                        os_path, os_mode_rb, errno, strerror(errno)););
        *err_info = FILE_ERROR;
#if FOPEN_OPENS_DIRECTORIES
      } else if ((file_no = os_fileno(in_fil)) != -1 &&
                 os_fstat(file_no, &stat_buf) == 0 &&
                 S_ISDIR(stat_buf.st_mode)) {
        logError(printf("openInfile: "
                        "fopen(\"" FMT_S_OS "\", \"" FMT_S_OS "\") "
                        "opened a directory.\n", os_path, os_mode_rb););
        fclose(in_fil);
        *err_info = FILE_ERROR;
#endif
      } else {
        if (unlikely(!ALLOC_FILE(new_file))) {
          fclose(in_fil);
          *err_info = MEMORY_ERROR;
        } else {
          name_ustri = (ustriType) stri_to_cstri8(sourceFileName, err_info);
          if (unlikely(name_ustri == NULL)) {
            logError(printf("openInfile: "
                            "stri_to_cstri8(\"%s\") failed:\n"
                            "err_info=%d\n",
                            striAsUnquotedCStri(sourceFileName),
                            *err_info););
            fclose(in_fil);
          } else {
            logMessage(printf("openInfile: name_ustri: \"%s\"\n",
                              name_ustri););
            name_length = strlen((cstriType) name_ustri);
            resized_name_ustri = REALLOC_USTRI(name_ustri,
                max_utf8_size(sourceFileName->size), name_length);
            /* Theoretical a 'realloc', which shrinks memory, should  */
            /* never fail. For the strange case that it fails we keep */
            /* name_ustri intact with the oversized memory size.      */
            if (resized_name_ustri != NULL) {
              name_ustri = resized_name_ustri;
            } /* if */
            if ((in_name = copy_stri(sourceFileName)) == NULL) {
              free_cstri8(name_ustri, sourceFileName);
              fclose(in_fil);
              *err_info = MEMORY_ERROR;
            } else {
              if (in_file.curr_infile != NULL) {
                memcpy(in_file.curr_infile, &in_file, sizeof(inFileRecord));
              } /* if */
              in_file.fil = in_fil;
              if (!speedup()) {
                fclose(in_file.fil);
                free_cstri8(name_ustri, sourceFileName);
                FREE_STRI(in_name);
                if (in_file.curr_infile != NULL) {
                  memcpy(&in_file, in_file.curr_infile, sizeof(inFileRecord));
                } else {
                  in_file.fil = NULL;
                } /* if */
                logError(printf("openInfile: speedup() failed.\n"
                                "os_path: \"" FMT_S_OS "\"\n", os_path););
                *err_info = FILE_ERROR;
              } else {
                COUNT_USTRI(name_length, count.fnam, count.fnam_bytes);
                in_file.name_ustri = name_ustri;
                in_file.name = in_name;
                in_file.path = absolutePath;
                in_file.character = next_character();
                in_file.line = 1;
                in_file.file_number = fileNumber;
                open_compilation_info(write_library_names, write_line_numbers);
                in_file.end_of_file = FALSE;
                in_file.up_infile = in_file.curr_infile;
                in_file.curr_infile = new_file;
                in_file.next = nextFile;
                memcpy(new_file, &in_file, sizeof(inFileRecord));
                isOpen = TRUE;
              } /* if */
            } /* if */
          } /* if */
        } /* if */
      } /* if */
      os_stri_free(os_path);
    } /* if */
    logFunction(printf("openInfile --> %d (err_info=%d)\n", isOpen, *err_info););
    return isOpen;
  } /* openInfile */



void closeInfile (void)

  { /* closeInfile */
    logFunction(printf("closeInfile (name=\"%s\")\n",
                       striAsUnquotedCStri(in_file.name)););
#if WITH_COMPILATION_INFO
    if (in_file.write_line_numbers) {
      NL_LIN_INFO();
    } else {
      if (in_file.write_library_names) {
        NL_FIL_LIN_INFO();
      } /* if */
    } /* if */
    total_lines = total_lines + ((unsigned long) in_file.line);
#endif
#if USE_ALTERNATE_NEXT_CHARACTER
    if (in_file.start != NULL) {
      if (in_file.buffer_size == 0) {
#if HAS_MMAP
        if (in_file.fil != NULL) {
          munmap(in_file.start, (size_t) (in_file.beyond - in_file.start));
        } /* if */
#endif
      } else {
        FREE_BYTES(in_file.start, in_file.buffer_size);
      } /* if */
    } /* if */
#endif
    if (in_file.fil != NULL) {
      fclose(in_file.fil);
      in_file.fil = NULL;
    } /* if */
#if !USE_ALTERNATE_NEXT_CHARACTER
#if USE_INFILE_BUFFER
    if (in_file.buffer != NULL) {
      FREE_BYTES(in_file.buffer, SIZE_IN_BUFFER);
    } /* if */
#endif
#endif
    if (in_file.curr_infile != NULL) {
      memcpy(in_file.curr_infile, &in_file, sizeof(inFileRecord));
    } /* if */
    if (in_file.up_infile != NULL) {
      memcpy(&in_file, in_file.up_infile, sizeof(inFileRecord));
      display_compilation_info();
    } else {
      in_file.curr_infile = NULL;
    } /* if */
    in_file.next_msg_line = in_file.line + in_file.incr_message_line;
    logFunction(printf("closeInfile -->\n"););
  } /* closeInfile */



boolType openBString (bstriType inputString,
    fileNumType fileNumber, inFileType nextFile,
    boolType write_library_names, boolType write_line_numbers,
    errInfoType *err_info)

  {
    const char sourceFileName[] = "STRING";
    inFileType new_file;
    memSizeType name_length;
    ustriType name_ustri;
    striType in_name;
    striType in_path;
    FILE *in_fil;
    boolType isOpen = FALSE;

  /* openBString */
    logFunction(printf("openBString(\"%s\", %d, %d, err_info=%d)\n",
                       bstriAsUnquotedCStri(inputString),
                       write_library_names, write_line_numbers,
                       *err_info););
    if (*err_info == OKAY_NO_ERROR) {
      if (unlikely(!ALLOC_FILE(new_file))) {
        *err_info = MEMORY_ERROR;
      } else {
        name_length = STRLEN(sourceFileName);
        if (unlikely(!ALLOC_USTRI(name_ustri, name_length))) {
          *err_info = MEMORY_ERROR;
        } else if (unlikely(!ALLOC_STRI_SIZE_OK(in_name, name_length))) {
          UNALLOC_USTRI(name_ustri, name_length);
          *err_info = MEMORY_ERROR;
        } else if (unlikely(!ALLOC_STRI_SIZE_OK(in_path, name_length))) {
          UNALLOC_USTRI(name_ustri, name_length);
          FREE_STRI2(in_name, name_length);
          *err_info = MEMORY_ERROR;
        } else {
#if USE_ALTERNATE_NEXT_CHARACTER
          in_fil = NULL;
#else
          in_fil = tmpfile();
          if (unlikely(in_fil == NULL)) {
            logError(printf("openBString: tmpfile() failed:\n"
                            "errno=%d\nerror: %s\n",
                            errno, strerror(errno)););
            *err_info = FILE_ERROR;
          } else {
            if (fwrite(inputString->mem, 1, inputString->size, in_fil) ==
                inputString->size) {
              rewind(in_fil);
            } else {
              logError(printf("openBString: writing to temporary file failed:\n"
                              "errno=%d\nerror: %s\n",
                              errno, strerror(errno)););
              *err_info = FILE_ERROR;
              fclose(in_fil);
              in_fil = NULL;
            } /* if */
          } /* if */
          if (unlikely(in_fil == NULL)) {
            UNALLOC_USTRI(name_ustri, name_length);
            FREE_STRI2(in_name, name_length);
            FREE_STRI2(in_path, name_length);
          } else
#endif
          {
            COUNT_USTRI(name_length, count.fnam, count.fnam_bytes);
            memcpy(name_ustri, sourceFileName, name_length);
            name_ustri[name_length] = '\0';
            in_name->size = name_length;
            memcpy_to_strelem(in_name->mem, name_ustri, name_length);
            in_path->size = name_length;
            memcpy_to_strelem(in_path->mem, name_ustri, name_length);
            if (in_file.curr_infile != NULL) {
              memcpy(in_file.curr_infile, &in_file, sizeof(inFileRecord));
            } /* if */
            in_file.fil = in_fil;
            in_file.name_ustri = name_ustri;
            in_file.name = in_name;
            in_file.path = in_path;
#if USE_ALTERNATE_NEXT_CHARACTER
            in_file.start = inputString->mem;
            in_file.nextch = in_file.start;
            in_file.beyond = in_file.start + inputString->size;
            in_file.buffer_size = 0;
#else
#if USE_INFILE_BUFFER
            in_file.buffer = NULL;
#endif
#endif
            in_file.character = next_character();
            in_file.line = 1;
            in_file.file_number = fileNumber;
            open_compilation_info(write_library_names, write_line_numbers);
            in_file.end_of_file = FALSE;
            in_file.up_infile = in_file.curr_infile;
            in_file.curr_infile = new_file;
            in_file.next = nextFile;
            memcpy(new_file, &in_file, sizeof(inFileRecord));
            isOpen = TRUE;
          }
        } /* if */
      } /* if */
    } /* if */
    logFunction(printf("openBString --> %d (err_info=%d)\n", isOpen, *err_info););
    return isOpen;
  } /* openBString */



static void freeFile (inFileType old_file)

  {
    memSizeType name_length;

  /* freeFile */
    logFunction(printf("freeFile(\"%s\"\n",
                       striAsUnquotedCStri(old_file->name)););
    name_length = strlen((const_cstriType) old_file->name_ustri);
    FREE_USTRI(old_file->name_ustri, name_length, count.fnam, count.fnam_bytes);
    FREE_STRI(old_file->name);
    FREE_STRI(old_file->path);
    FREE_FILE(old_file);
    logFunction(printf("freeFile -->\n"););
  } /* freeFile */



void removeProgFiles (progType aProg)

  {
    inFileType aFile;
    inFileType currFile;

  /* removeProgFiles */
    logFunction(printf("removeProgFiles(" FMT_U_MEM ")\n",
                       (memSizeType) aProg););
    aFile = aProg->fileList;
    while (aFile != NULL) {
      currFile = aFile;
      aFile = aFile->next;
      freeFile(currFile);
    } /* while */
    if (question_mark != NULL) {
      FREE_STRI(question_mark);
      question_mark = NULL;
    } /* if */
    logFunction(printf("removeProgFiles -->\n"););
  } /* removeProgFiles */



void next_file (void)

  { /* next_file */
    logFunction(printf("next_file\n"););
    if (in_file.up_infile != NULL) {
      closeInfile();
    } else {
      in_file.end_of_file = TRUE;
    } /* if */
    logFunction(printf("next_file -->\n"););
  } /* next_file */



#ifdef OUT_OF_ORDER
int next_line (void)

  {
    register int character;

  /* next_line */
    logFunction(printf("next_line\n"););
    SKIP_CR_SP(character);
    INCR_LINE_COUNT(in_file.line);
    logFunction(printf("next_line -->\n"););
    return character;
  } /* next_line */
#endif



striType get_file_name (progType aProg, fileNumType file_num)

  {
    register inFileType help_file;
    striType file_name;

  /* get_file_name */
    logFunction(printf("get_file_name(" FMT_U_MEM ", %u)\n",
                       (memSizeType) aProg, file_num););
    help_file = aProg->fileList;
    while (help_file != NULL && help_file->file_number != file_num) {
      help_file = help_file->next;
    } /* while */
    if (help_file != NULL) {
      file_name = help_file->name;
    } else {
      if (question_mark == NULL) {
        question_mark = CSTRI_LITERAL_TO_STRI("?");
      } /* if */
      file_name = question_mark;
    } /* if */
    logFunction(printf("get_file_name --> \"%s\"\n",
                       striAsUnquotedCStri(file_name)););
    return file_name;
  } /* get_file_name */



striType objectFileName (const_objectType anObject)

  {
    fileNumType fileNumber;
    striType fileName;

  /* objectFileName */
    logFunction(printf("objectFileName(" FMT_U_MEM ")\n",
                       (memSizeType) anObject););
    if (HAS_POSINFO(anObject)) {
      fileNumber = POSINFO_FILE_NUM(anObject);
    } else if (HAS_PROPERTY(anObject)) {
      fileNumber = PROPERTY_FILE_NUM(anObject);
    } else {
      fileNumber = 0;
    } /* if */
    if (fileNumber != 0 &&
        anObject->type_of != NULL &&
        anObject->type_of->owningProg != NULL) {
      fileName = get_file_name(anObject->type_of->owningProg,
                               fileNumber);
    } else {
      if (question_mark == NULL) {
        question_mark = CSTRI_LITERAL_TO_STRI("?");
      } /* if */
      fileName = question_mark;
    } /* if */
    logFunction(printf("objectFileName --> \"%s\"\n",
                       striAsUnquotedCStri(fileName)););
    return fileName;
  } /* objectFileName */



static striType get_file_path (progType aProg, fileNumType file_num)

  {
    register inFileType help_file;
    striType file_path;

  /* get_file_path */
    logFunction(printf("get_file_path(" FMT_U_MEM ", %u)\n",
                       (memSizeType) aProg, file_num););
    help_file = aProg->fileList;
    while (help_file != NULL && help_file->file_number != file_num) {
      help_file = help_file->next;
    } /* while */
    if (help_file != NULL) {
      file_path = help_file->path;
    } else {
      if (question_mark == NULL) {
        question_mark = CSTRI_LITERAL_TO_STRI("?");
      } /* if */
      file_path = question_mark;
    } /* if */
    logFunction(printf("get_file_path --> \"%s\"\n",
                       striAsUnquotedCStri(file_path)););
    return file_path;
  } /* get_file_path */



striType objectFilePath (const_objectType anObject)

  {
    fileNumType fileNumber;
    striType filePath;

  /* objectFilePath */
    logFunction(printf("objectFilePath(" FMT_U_MEM ")\n",
                       (memSizeType) anObject););
    if (HAS_POSINFO(anObject)) {
      fileNumber = POSINFO_FILE_NUM(anObject);
    } else if (HAS_PROPERTY(anObject)) {
      fileNumber = PROPERTY_FILE_NUM(anObject);
    } else {
      fileNumber = 0;
    } /* if */
    if (fileNumber != 0 &&
        anObject->type_of != NULL &&
        anObject->type_of->owningProg != NULL) {
      filePath = get_file_path(anObject->type_of->owningProg,
                               fileNumber);
    } else {
      if (question_mark == NULL) {
        question_mark = CSTRI_LITERAL_TO_STRI("?");
      } /* if */
      filePath = question_mark;
    } /* if */
    logFunction(printf("objectFilePath --> \"%s\"\n",
                       striAsUnquotedCStri(filePath)););
    return filePath;
  } /* objectFilePath */



const_ustriType get_file_name_ustri (progType aProg, fileNumType file_num)

  {
    inFileType help_file;
    const_ustriType file_name;

  /* get_file_name_ustri */
    logFunction(printf("get_file_name_ustri(" FMT_U_MEM ", %u)\n",
                       (memSizeType) aProg, file_num););
    help_file = aProg->fileList;
    while (help_file != NULL && help_file->file_number != file_num) {
      help_file = help_file->next;
    } /* while */
    if (help_file != NULL) {
      file_name = help_file->name_ustri;
    } else {
      file_name = (const_ustriType) "?";
    } /* if */
    logFunction(printf("get_file_name_ustri --> \"%s\"\n", file_name););
    return file_name;
  } /* get_file_name_ustri */
