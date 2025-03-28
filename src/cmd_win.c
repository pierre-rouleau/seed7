/********************************************************************/
/*                                                                  */
/*  cmd_win.c     Command functions which call the Windows API.     */
/*  Copyright (C) 1989 - 2016, 2020, 2021  Thomas Mertes            */
/*                2023 - 2025  Thomas Mertes                        */
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
/*  File: seed7/src/cmd_win.c                                       */
/*  Changes: 2011 - 2016, 2020, 2021, 2023 - 2025  Thomas Mertes    */
/*  Content: Command functions which call the Windows API.          */
/*                                                                  */
/********************************************************************/

#define LOG_FUNCTIONS 0
#define VERBOSE_EXCEPTIONS 0

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "windows.h"
#if ACLAPI_H_PRESENT
#include "aclapi.h"
#endif
#include "io.h"
#include "fcntl.h"
#ifdef OS_STRI_WCHAR
#include "wchar.h"
#endif
#include "errno.h"

#include "common.h"
#include "data_rtl.h"
#include "heaputl.h"
#include "striutl.h"
#include "tim_rtl.h"
#include "str_rtl.h"
#include "tim_drv.h"
#include "rtl_err.h"

#undef EXTERN
#define EXTERN
#include "cmd_drv.h"


#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

#if !ACLAPI_H_PRESENT
#define SE_FILE_OBJECT 1
DWORD GetNamedSecurityInfoW (LPCWSTR pObjectName,
    int objectType, SECURITY_INFORMATION securityInfo,
    PSID *ppsidOwner, PSID *ppsidGroup,
    PACL *ppDacl, PACL *ppSacl,
    PSECURITY_DESCRIPTOR *ppSecurityDescriptor);
DWORD SetNamedSecurityInfoW (LPWSTR pObjectName,
    int objectType, SECURITY_INFORMATION securityInfo,
    PSID psidOwner, PSID psidGroup,
    PACL pDacl, PACL pSacl);
#endif

static SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
static PSID administratorSid = NULL;
static SID_IDENTIFIER_AUTHORITY worldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
static PSID worldSid = NULL;

#ifdef HAS_SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#define SYMBOLIC_LINK_FLAG SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#else
#define SYMBOLIC_LINK_FLAG 0
#endif



#if defined OS_STRI_WCHAR && !defined USE_WMAIN
#ifdef DEFINE_COMMAND_LINE_TO_ARGV_W
#define CommandLineToArgvW MyCommandLineToArgvW
/**
 *  Special handling of backslash characters for CommandLineToArgvW.
 *  CommandLineToArgvW reads arguments in two modes. Inside and
 *  outside quotation mode. The following rules apply if a
 *  backslash is encountered:
 *  - 2n backslashes followed by a quotation mark produce
 *    n backslashes and a switch from inside to outside quotation
 *    mode and vice versa. In this case the quotation mark is not
 *    added to the argument.
 *  - (2n) + 1 backslashes followed by a quotation mark produce
 *    n backslashes followed by a quotation mark. In this case the
 *    quotation mark is added to the argument and the quotation mode
 +    is not changed.
 *  - n backslashes not followed by a quotation mark simply produce
 *    n backslashes.
 */
static void processBackslash (const_os_striType *sourcePos, os_striType *destPos)

  {
    memSizeType backslashCount;
    memSizeType count;

  /* processBackslash */
    backslashCount = 1;
    (*sourcePos)++;
    while (**sourcePos == '\\') {
      backslashCount++;
      (*sourcePos)++;
    } /* while */
    if (**sourcePos == '"') {
      /* Backslashes in the result: backslashCount / 2   */
      for (count = backslashCount >> 1; count > 0; count--) {
        **destPos = '\\';
        (*destPos)++;
      } /* for */
      if (backslashCount & 1) {
        /* Add a quotation mark (") to the result.       */
        **destPos = '"';
        (*destPos)++;
        /* Stay in current quotation mode                */
        (*sourcePos)++;
      } else {
        /* Ignore the quotation mark (").                */
        /* Switch from inside to outside quotation mode  */
        /* and vice versa.                               */
      } /* if */
    } else {
      /* N backslashes not followed by a quotation mark  */
      /* simply produce n backslashes.                   */
      for (count = backslashCount; count > 0; count--) {
        **destPos = '\\';
        (*destPos)++;
      } /* for */
    } /* if */
  } /* processBackslash */



/**
 *  Parse commandLine and generate an array of pointers to the arguments.
 *  The parameter w_argc and the returned array of pointers (w_argv)
 *  correspond to the parameters argc and argv of main().
 *  The rules to recognize the first argument (the command) are
 *  different from the rules to recognize the other (normal) arguments.
 *  Arguments can be quoted or unquoted. Normal arguments (all except
 *  the first argument) can consist of quoted and unquoted parts. The
 *  quoted and unquoted parts that are concatenated to form one argument.
 *  To handle quoted and unquoted parts the function works with two
 *  modes: Inside and outside quotation mode.
 *  @param w_argc Address to which the argument count is copied.
 *  @return an array of pointers to the arguments of commandLine.
 */
static os_striType *CommandLineToArgvW (const_os_striType commandLine, int *w_argc)

  {
    size_t command_line_size;
    const_os_striType sourcePos;
    os_striType destPos;
    os_striType destBuffer;
    memSizeType argumentCount;
    os_striType *w_argv;

  /* CommandLineToArgvW */
    command_line_size = os_stri_strlen(commandLine);
    argumentCount = 0;
    w_argv = (os_striType *) malloc(command_line_size * sizeof(os_striType *));
    if (w_argv != NULL) {
      sourcePos = commandLine;
      while (*sourcePos == ' ') {
        sourcePos++;
      } /* while */
      if (*sourcePos == 0) {
        w_argv[0] = NULL;
      } else {
        if (unlikely(!os_stri_alloc(destBuffer, command_line_size))) {
          free(w_argv);
          w_argv = NULL;
        } else {
          /* Set pointer to first char of first argument */
          w_argv[0] = destBuffer;
          argumentCount = 1;
          destPos = destBuffer;
          if (*sourcePos == '"') {
            sourcePos++;
            while (*sourcePos != '"' && *sourcePos != 0) {
              *destPos = *sourcePos;
              sourcePos++;
              destPos++;
            } /* if */
          } else {
            while (*sourcePos != ' ' && *sourcePos != 0) {
              *destPos = *sourcePos;
              sourcePos++;
              destPos++;
            } /* if */
          } /* if */
          if (*sourcePos != 0) {
            do {
              sourcePos++;
            } while (*sourcePos == ' ');
            if (*sourcePos != 0) {
              /* Terminate the current argument */
              *destPos = 0;
              destPos++;
              /* Set pointer to first char of next argument */
              w_argv[argumentCount] = destPos;
              argumentCount++;
            } /* if */
          } /* if */
          while (*sourcePos != 0) {
            /* printf("source char: %d\n", *sourcePos); */
            if (*sourcePos == '"') {
              /* Inside quotation mode */
              sourcePos++;
              while (*sourcePos != '"' && *sourcePos != 0) {
                if (*sourcePos == '\\') {
                  processBackslash(&sourcePos, &destPos);
                } else {
                  *destPos = *sourcePos;
                  sourcePos++;
                  destPos++;
                } /* if */
              } /* while */
              if (*sourcePos == '"') {
                /* Consume the terminating quotation mark */
                sourcePos++;
              } /* if */
            } /* if */
            if (*sourcePos != ' ' && *sourcePos != 0) {
              /* Outside quotation mode */
              do {
                if (*sourcePos == '\\') {
                  processBackslash(&sourcePos, &destPos);
                } else {
                  *destPos = *sourcePos;
                  sourcePos++;
                  destPos++;
                } /* if */
              } while (*sourcePos != ' ' && *sourcePos != '"' && *sourcePos != 0);
            } /* if */
            if (*sourcePos == ' ') {
              do {
                sourcePos++;
              } while (*sourcePos == ' ');
              if (*sourcePos != 0) {
                /* Terminate the current argument */
                *destPos = 0;
                destPos++;
                /* Set pointer to first char of next argument */
                w_argv[argumentCount] = destPos;
                argumentCount++;
              } /* if */
            } /* if */
          } /* while */
          /* Terminate the last argument */
          *destPos = 0;
          w_argv[argumentCount] = NULL;
        } /* if */
      } /* if */
    } /* if */
    *w_argc = argumentCount;
    return w_argv;
  } /* CommandLineToArgvW */



void freeUtf16Argv (os_striType *w_argv)

  { /* freeUtf16Argv */
    if (w_argv != NULL) {
      os_stri_free(w_argv[0]);
      free(w_argv);
    } /* if */
  } /* freeUtf16Argv */

#else



void freeUtf16Argv (os_striType *w_argv)

  { /* freeUtf16Argv */
    LocalFree(w_argv);
  } /* freeUtf16Argv */
#endif



os_striType *getUtf16Argv (int *w_argc)

  {
    os_striType commandLine;
    os_striType *w_argv;

  /* getUtf16Argv */
    commandLine = GetCommandLineW();
    w_argv = CommandLineToArgvW(commandLine, w_argc);
    return w_argv;
  } /* getUtf16Argv */
#endif



/**
 *  Get the absolute path of the executable of the current process.
 *  @param arg_0 Parameter argv[0] from the function main() as string.
 *  @return the absolute path of the current process.
 */
striType getExecutablePath (const const_striType arg_0)

  {
    os_charType buffer[PATH_MAX];
    errInfoType err_info = OKAY_NO_ERROR;
    striType executablePath;

  /* getExecutablePath */
    if (unlikely(GetModuleFileNameW(NULL, buffer, PATH_MAX) == 0)) {
      raise_error(FILE_ERROR);
      executablePath = NULL;
    } else {
      executablePath = cp_from_os_path(buffer, &err_info);
      if (unlikely(executablePath == NULL)) {
        raise_error(err_info);
      } /* if */
    } /* if */
    return executablePath;
  } /* getExecutablePath */



#if USE_GET_ENVIRONMENT
os_striType *getEnvironment (void)

  {
    os_striType envBuffer;
    os_striType currPos;
    memSizeType length;
    memSizeType numElems = 0;
    memSizeType currIdx = 0;
    os_striType *env;

  /* getEnvironment */
    logFunction(printf("getEnvironment()"););
    envBuffer = GetEnvironmentStringsW();
    if (envBuffer == NULL) {
      env = NULL;
    } else {
      /* printf("envBuffer: \"" FMT_S_OS "\"\n", envBuffer); */
      currPos = envBuffer;
      do {
        length = os_stri_strlen(currPos);
        currPos = &currPos[length + 1];
        numElems++;
      } while (length != 0);
      /* printf("numElems: " FMT_U_MEM "\n", numElems); */
      env = (os_striType *) malloc(numElems * sizeof(os_striType));
      if (env != NULL) {
        currPos = envBuffer;
        do {
          env[currIdx] = currPos;
          length = os_stri_strlen(currPos);
          currPos = &currPos[length + 1];
          currIdx++;
        } while (length != 0);
        env[currIdx - 1] = NULL;
        /* for (currIdx = 0; env[currIdx] != NULL; currIdx++) {
          printf("env[" FMT_U_MEM "]: \"" FMT_S_OS "\"\n", currIdx, env[currIdx]);
        } */
      } /* if */
      if (env == NULL || env[0] == NULL) {
        if (FreeEnvironmentStringsW(envBuffer) == 0) {
          logError(printf("getEnvironment: FreeEnvironmentStrings() failed.\n"););
        } /* if */
      } /* if */
    } /* if */
    return env;
  }  /* getEnvironment */



void freeEnvironment (os_striType *environment)

  { /* freeEnvironment */
    if (environment != NULL) {
      if (environment[0] != NULL) {
        if (FreeEnvironmentStringsW(environment[0]) == 0) {
          logError(printf("getEnvironment: FreeEnvironmentStrings() failed.\n"););
        } /* if */
      } /* if */
      free(environment);
    } /* if */
  } /* freeEnvironment */
#endif



#ifdef DEFINE_WGETENV
os_striType wgetenv (const const_os_striType name)

  {
    memSizeType value_size;
    os_striType value;

  /* wgetenv */
    value_size = GetEnvironmentVariableW(name, NULL, 0);
    if (value_size == 0) {
      value = NULL;
    } else {
      if (ALLOC_UTF16(value, value_size - 1)) {
        if (GetEnvironmentVariableW(name, value, value_size) != value_size - 1) {
          FREE_OS_STRI(value);
          value = NULL;
        } /* if */
      } /* if */
    } /* if */
    return value;
  } /* wgetenv */
#endif



#ifdef DEFINE_WSETENV
int wsetenv (const const_os_striType name, const const_os_striType value,
    int overwrite)

  {
    int result;

  /* wsetenv */
    logFunction(printf("wsetenv(\"" FMT_S_OS "\", \"" FMT_S_OS "\", &d)\n",
                       name, value, overwrite););
    result = !SetEnvironmentVariableW(name, value);
    return result;
  } /* wsetenv */
#endif



#ifdef DEFINE_WUNSETENV
int wunsetenv (const const_os_striType name)

  {
    int result;

  /* wunsetenv */
    logFunction(printf("wunsetenv(\"" FMT_S_OS "\")\n", name););
    result = !SetEnvironmentVariableW(name, NULL);
    return result;
  } /* wunsetenv */
#endif



#ifdef DEFINE_WIN_RENAME
/**
 *  Renames a file, moving it between directories if required.
 *  This function is a replacement for _wrename(), which does not work correctly.
 *  Unlike the Linux/Unix/BSD rename() function the Windows _wrename() function
 *  follows symbolic links and renames the link destination. This makes it
 *  impossible to rename symbolic links with _wrename(). This function uses
 *  MoveFileExW(), which does not follow symbolic links like the rename()
 *  functions of Linux/Unix/BSD.
 */
int winRename (const const_os_striType oldPath, const const_os_striType newPath)

  {
    DWORD lastError;
    int result;

  /* winRename */
    logFunction(printf("winRename(\"" FMT_S_OS "\", \"" FMT_S_OS "\")\n",
                       oldPath, newPath););
    if (unlikely(MoveFileExW(oldPath, newPath, MOVEFILE_REPLACE_EXISTING)) == 0) {
      lastError = GetLastError();
      logError(printf("winRename(\"" FMT_S_OS "\", \"" FMT_S_OS "\"): "
                      "MoveFileW failed:\nlastError=" FMT_U32 "\n",
                      oldPath, newPath, (uint32Type) lastError););
      if (lastError == ERROR_FILE_NOT_FOUND ||
          lastError == ERROR_PATH_NOT_FOUND) {
        errno = ENOENT;
      } else if (lastError == ERROR_NOT_SAME_DEVICE) {
#ifdef EXDEV
        errno = EXDEV;
#else
        errno = EACCES;
#endif
      } else if (lastError == ERROR_PRIVILEGE_NOT_HELD) {
        errno = EACCES;
      } /* if */
      result = -1;
    } else {
      result = 0;
    } /* if */
    logFunction(printf("winRename (\"" FMT_S_OS "\", \"" FMT_S_OS "\") --> %d\n",
                       oldPath, newPath, result););
    return result;
  } /* winRename */
#endif



#ifdef DEFINE_WIN_SYMLINK
static int wSymlink (const const_os_striType targetPath, const const_os_striType symlinkPath)

  {
    int result;

  /* wSymlink */
    logFunction(printf("wSymlink(\"" FMT_S_OS "\", \"" FMT_S_OS "\")\n",
                       targetPath, symlinkPath););
    if (unlikely(CreateSymbolicLinkW(symlinkPath, targetPath,
                                     SYMBOLIC_LINK_FLAG) == 0)) {
      logError(printf("wSymlink(\"" FMT_S_OS "\", \"" FMT_S_OS "\"): "
                      "CreateSymbolicLinkW(\"" FMT_S_OS "\", \"" FMT_S_OS
                                           "\", " FMT_U32 ") failed:\n"
                      "lastError=" FMT_U32 "\n",
                      targetPath, symlinkPath, symlinkPath, targetPath,
                      (uint32Type) SYMBOLIC_LINK_FLAG,
                      (uint32Type) GetLastError()););
      result = -1;
    } else {
      result = 0;
    } /* if */
    logFunction(printf("wSymlink(\"" FMT_S_OS "\", \"" FMT_S_OS "\") --> %d\n",
                       targetPath, symlinkPath, result););
    return result;
  } /* wSymlink */



void winSymlink (const const_striType targetPath,
    const const_striType symlinkPath, errInfoType *err_info)
  {
    os_striType os_targetPath;
    os_striType symlinkTargetPath;
    os_striType os_symlinkPath;
    int path_info;

  /* winSymlink */
    logFunction(printf("winSymlink(\"%s\", ",
                       striAsUnquotedCStri(targetPath));
                printf("\"%s\", %d)\n",
                       striAsUnquotedCStri(symlinkPath), *err_info););
    if (targetPath->size > 0 && targetPath->mem[0] == '/') {
      /* Create a symbolic link to an absolute target. */
      os_targetPath = cp_to_os_path(targetPath, &path_info, err_info);
    } else
#if FORBID_DRIVE_LETTERS
    if (unlikely(targetPath->size >= 2 && (targetPath->mem[targetPath->size - 1] == '/' ||
                 (targetPath->mem[1] == ':' &&
                 ((targetPath->mem[0] >= 'a' && targetPath->mem[0] <= 'z') ||
                  (targetPath->mem[0] >= 'A' && targetPath->mem[0] <= 'Z')))))) {
#else
    if (unlikely(targetPath->size >= 2 && targetPath->mem[targetPath->size - 1] == '/')) {
#endif
      logError(printf("winSymlink(\"%s\", ...): "
                      "Target path with drive letters or not legal.\n",
                      striAsUnquotedCStri(targetPath)););
      *err_info = RANGE_ERROR;
      os_targetPath = NULL;
    } else if (unlikely(memchr_strelem(targetPath->mem, '\\', targetPath->size) != NULL)) {
      logError(printf("winSymlink(\"%s\", ...): Target path contains a backslash.\n",
                      striAsUnquotedCStri(targetPath)););
      *err_info = RANGE_ERROR;
      os_targetPath = NULL;
    } else {
      os_targetPath = stri_to_os_stri(targetPath, err_info);
    } /* if */
    if (likely(os_targetPath != NULL)) {
#if defined USE_EXTENDED_LENGTH_PATH && USE_EXTENDED_LENGTH_PATH
      if (memcmp(os_targetPath, PATH_PREFIX, PREFIX_LEN * sizeof(os_charType)) == 0) {
        /* Omit the extended path prefix from the target path. */
        symlinkTargetPath = &os_targetPath[PREFIX_LEN];
      } else {
        symlinkTargetPath = os_targetPath;
      } /* if */
#else
      symlinkTargetPath = os_targetPath;
#endif
      os_symlinkPath = cp_to_os_path(symlinkPath, &path_info, err_info);
      if (likely(os_symlinkPath != NULL)) {
        if (unlikely(wSymlink(symlinkTargetPath, os_symlinkPath) != 0)) {
          *err_info = FILE_ERROR;
        } /* if */
        os_stri_free(os_symlinkPath);
      } /* if */
      os_stri_free(os_targetPath);
    } /* if */
    logFunction(printf("winSymlink(\"%s\", ",
                       striAsUnquotedCStri(targetPath));
                printf("\"%s\", %d) -->\n",
                       striAsUnquotedCStri(symlinkPath), *err_info););
  } /* winSymlink */
#endif



#ifdef HAS_DEVICE_IO_CONTROL

typedef struct {
    ULONG ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      union {
        struct {
          ULONG Flags;
          WCHAR PathBuffer[1];
        } SymbolicLink;
        struct {
          WCHAR PathBuffer[1];
        } MountPoint;
      } Data;
    } Destination;
  } REPARSE_DATA_BUFFER7;

#ifndef FSCTL_GET_REPARSE_POINT
#define FSCTL_GET_REPARSE_POINT 0x900a8
#endif
#ifndef IO_REPARSE_TAG_SYMLINK
#define IO_REPARSE_TAG_SYMLINK 0xa000000c
#endif
#ifndef IO_REPARSE_TAG_MOUNT_POINT
#define IO_REPARSE_TAG_MOUNT_POINT 0xa0000003
#endif

#define DRIVE_NAME_LENGTH   2 /* Length of c: */
#define DRIVE_PREFIX_LENGTH 6 /* Length of \\?\c: */
#define DRIVE_LETTER_INDEX  4 /* Index of c in \\?\c: */



static striType getSymlinkDestination (const wchar_t *osSymlinkPath,
    const wchar_t *substituteName, USHORT substituteNameByteLen,
    errInfoType *err_info)

  {
    USHORT substituteNameLength;
    wchar_t *osTargetPath;
    striType destination;

  /* getSymlinkDestination */
    logFunction(printf("getSymlinkDestination(\"%ls\", \"%.*ls\", %hu, %d)\n",
                       osSymlinkPath,
                       (int) substituteNameByteLen / sizeof(wchar_t),
                       substituteName, substituteNameByteLen, *err_info););
    substituteNameLength = substituteNameByteLen / sizeof(wchar_t);
    if (substituteNameLength >= 1 && substituteName[0] == (wchar_t) '\\') {
      /* An absolute substitute name starts with \??\c:\    */
      /* (assuming that the drive letter of the path is c). */
      /* Our check allows also variants of this prefix.     */
      if (substituteNameLength < 7 ||
          substituteName[2] != (wchar_t) '?'  ||
          substituteName[3] != (wchar_t) '\\' ||
          substituteName[5] != (wchar_t) ':'  ||
          substituteName[6] != (wchar_t) '\\') {
        /* This is a root relative symbolic link. It is relative */
        /* to the drive of the symlink (and not relative to the  */
        /* current working drive). A root relative symbolic link */
        /* is converted to an absolute link. The root relative   */
        /* substitute name is concatenated to the drive taken    */
        /* from osSymlinkPath (=directory of the symlink). Only  */
        /* Windows has root relative symbolic links.             */
        if (unlikely(!ALLOC_OS_STRI(osTargetPath,
                      (memSizeType) DRIVE_NAME_LENGTH + substituteNameLength))) {
          *err_info = MEMORY_ERROR;
          destination = NULL;
        } else {
          memcpy(osTargetPath, &osSymlinkPath[PREFIX_LEN],
                 DRIVE_NAME_LENGTH * sizeof(wchar_t));
          memcpy(&osTargetPath[DRIVE_NAME_LENGTH],
                 substituteName, substituteNameByteLen);
          destination = cp_from_os_path_buffer(osTargetPath,
              (memSizeType) DRIVE_NAME_LENGTH + substituteNameLength,
              err_info);
          FREE_OS_STRI(osTargetPath);
        } /* if */
      } else {
        /* Skip the prefix \??\ */
        destination = cp_from_os_path_buffer(&substituteName[PREFIX_LEN],
            substituteNameLength - PREFIX_LEN, err_info);
      } /* if */
    } else {
      /* Relative substitute name */
      destination = cp_from_os_path_buffer(substituteName,
          substituteNameLength, err_info);
    } /* if */
    logFunction(printf("getSymlinkDestination --> \"%s\"\n",
                       striAsUnquotedCStri(destination)););
    return destination;
  } /* getSymlinkDestination */



/**
 *  Reads the destination of a symbolic link.
 *  @param err_info Unchanged if the function succeeds, and
 *                  MEMORY_ERROR if a memory allocation failed, and
 *                  RANGE_ERROR if the conversion to the system path failed, and
 *                  FILE_ERROR if the file does not exist or is not a symbolic link.
 *  @return The destination referred by the symbolic link, or
 *          NULL if an error occurred.
 */
striType winReadLink (const const_striType filePath, errInfoType *err_info)

  {
    os_striType os_filePath;
    int path_info;
    HANDLE fileHandle;
    union info_t {
      char buffer[100]; /* Arbitrary buffer size (must be >= 28) */
      REPARSE_DATA_BUFFER7 reparseData;
    } info;
    memSizeType dataBufferHeadLength;
    memSizeType dataBufferLength;
    REPARSE_DATA_BUFFER7 *reparseData;
    DWORD bytesReturned;
    DWORD lastError;
    WCHAR *pathBuffer;
    striType destination = NULL;

  /* winReadLink */
    logFunction(printf("winReadLink(\"%s\", %d)\n",
                       striAsUnquotedCStri(filePath), *err_info););
    os_filePath = cp_to_os_path(filePath, &path_info, err_info);
    if (unlikely(os_filePath == NULL)) {
      logError(printf("winReadLink: cp_to_os_path(\"%s\", *, *) failed:\n"
                      "path_info=%d, err_info=%d\n",
                      striAsUnquotedCStri(filePath), path_info, *err_info););
    } else {
      fileHandle = CreateFileW(os_filePath, 0,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT |
                               FILE_FLAG_BACKUP_SEMANTICS, NULL);
      if (unlikely(fileHandle == INVALID_HANDLE_VALUE)) {
        logError(printf("winReadLink(\"%s\", *): "
                        "CreateFileW(\"" FMT_S_OS "\", *) failed:\n"
                        "lastError=" FMT_U32 "\n",
                        striAsUnquotedCStri(filePath), os_filePath,
                        (uint32Type) GetLastError()););
        *err_info = FILE_ERROR;
      } else {
        if (unlikely(DeviceIoControl(fileHandle,
                                     FSCTL_GET_REPARSE_POINT,
                                     NULL, 0, info.buffer, sizeof(info),
                                     &bytesReturned, NULL) == 0)) {
          lastError = GetLastError();
          if (lastError != ERROR_MORE_DATA) {
            logError(printf("winReadLink(\"%s\", *): "
                            "DeviceIoControl(" FMT_U_MEM ", ...) failed:\n"
                            "lastError=" FMT_U32 "%s\n",
                            striAsUnquotedCStri(filePath),
                            (memSizeType) fileHandle,
                            (uint32Type) lastError,
                            lastError == ERROR_NOT_A_REPARSE_POINT ?
                                " (ERROR_NOT_A_REPARSE_POINT)" : ""););
            *err_info = FILE_ERROR;
          } else if (unlikely(info.reparseData.ReparseTag !=
                              IO_REPARSE_TAG_SYMLINK &&
                              info.reparseData.ReparseTag !=
                              IO_REPARSE_TAG_MOUNT_POINT)) {
            logError(printf("winReadLink(\"%s\", *): "
                            "Unexpected ReparseTag: 0x" FMT_X32 "\n",
                            striAsUnquotedCStri(filePath),
                            (uint32Type) info.reparseData.ReparseTag););
            *err_info = FILE_ERROR;
          } else {
            dataBufferHeadLength = (memSizeType)
                ((char *) &info.reparseData.Destination -
                 (char *) &info.reparseData);
            dataBufferLength = dataBufferHeadLength +
                info.reparseData.ReparseDataLength;
            reparseData = (REPARSE_DATA_BUFFER7 *) malloc(dataBufferLength);
            if (unlikely(reparseData == NULL)) {
              *err_info = MEMORY_ERROR;
            } else {
              if (unlikely(DeviceIoControl(fileHandle,
                                           FSCTL_GET_REPARSE_POINT, NULL, 0,
                                           reparseData, (DWORD) dataBufferLength,
                                           &bytesReturned, NULL) == 0)) {
                logError(printf("winReadLink(\"%s\", *): "
                                "DeviceIoControl(" FMT_U_MEM ", ...) failed:\n"
                                "lastError=" FMT_U32 "\n",
                                striAsUnquotedCStri(filePath),
                                (memSizeType) fileHandle,
                                (uint32Type) GetLastError()););
                *err_info = FILE_ERROR;
              } else {
                if (reparseData->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
                  pathBuffer = reparseData->Destination.Data.SymbolicLink.PathBuffer;
                } else if (reparseData->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
                  pathBuffer = reparseData->Destination.Data.MountPoint.PathBuffer;
                } else {
                  logError(printf("winReadLink(\"%s\", *): "
                                  "Unexpected ReparseTag: 0x" FMT_X32 "\n",
                                  striAsUnquotedCStri(filePath),
                                  (uint32Type) reparseData->ReparseTag););
                  *err_info = FILE_ERROR;
                  pathBuffer = NULL;
                } /* if */
                if (likely(pathBuffer != NULL)) {
                  destination = getSymlinkDestination(os_filePath,
                      &pathBuffer[reparseData->Destination.SubstituteNameOffset /
                          sizeof(wchar_t)],
                      reparseData->Destination.SubstituteNameLength,
                      err_info);
                } /* if */
              } /* if */
              free(reparseData);
            } /* if */
          } /* if */
        } else {
          if (info.reparseData.ReparseTag == IO_REPARSE_TAG_SYMLINK) {
            pathBuffer = info.reparseData.Destination.Data.SymbolicLink.PathBuffer;
          } else if (info.reparseData.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
            pathBuffer = info.reparseData.Destination.Data.MountPoint.PathBuffer;
          } else {
            logError(printf("winReadLink(\"%s\", *): "
                            "Unexpected ReparseTag: 0x" FMT_X32 "\n",
                            striAsUnquotedCStri(filePath),
                            (uint32Type) info.reparseData.ReparseTag););
            *err_info = FILE_ERROR;
            pathBuffer = NULL;
          } /* if */
          if (likely(pathBuffer != NULL)) {
            destination = getSymlinkDestination(os_filePath,
                &pathBuffer[info.reparseData.Destination.SubstituteNameOffset /
                    sizeof(wchar_t)],
                info.reparseData.Destination.SubstituteNameLength,
                err_info);
          } /* if */
        } /* if */
        CloseHandle(fileHandle);
      } /* if */
      os_stri_free(os_filePath);
    } /* if */
    logFunction(printf("winReadLink(\"%s\", %d) --> ",
                       striAsUnquotedCStri(filePath), *err_info);
                printf("\"%s\"\n", striAsUnquotedCStri(destination)););
    return destination;
  } /* winReadLink */



const wchar_t *winFollowSymlink (const wchar_t *path, int numberOfFollowsAllowed);



static const wchar_t *followSymlinkRecursive (const wchar_t *osSymlinkPath,
    const wchar_t *substituteName, USHORT substituteNameByteLen,
    int numberOfFollowsAllowed)

  {
    USHORT substituteNameLength;
    wchar_t *lastBackslashPos;
    memSizeType directoryPathLength;
    wchar_t *destination;
    const wchar_t *result;

  /* followSymlinkRecursive */
    logFunction(printf("followSymlinkRecursive(\"%ls\", \"%.*ls\", %hu, %d)\n",
                       osSymlinkPath,
                       (int) (substituteNameByteLen / sizeof(wchar_t)),
                       substituteName, substituteNameByteLen,
                       numberOfFollowsAllowed););
    substituteNameLength = substituteNameByteLen / sizeof(wchar_t);
    if (substituteNameLength >= 1 && substituteName[0] == (wchar_t) '\\') {
      /* An absolute substitute name starts with \??\c:\    */
      /* (assuming that the drive letter of the path is c). */
      /* Our check allows also variants of this prefix.     */
      /* For the recursive call we need a prefix of \\?\c:\ */
      /* (=PATH_PREFIX) instead.                            */
      if (substituteNameLength < 7 ||
          substituteName[2] != (wchar_t) '?'  ||
          substituteName[3] != (wchar_t) '\\' ||
          substituteName[5] != (wchar_t) ':'  ||
          substituteName[6] != (wchar_t) '\\') {
        /* This is a root relative symbolic link. It is relative */
        /* to the drive of the symlink (and not relative to the  */
        /* current working drive). A root relative symbolic link */
        /* is converted to an absolute link. The root relative   */
        /* substitute name is concatenated to the drive taken    */
        /* from osSymlinkPath (=directory of the symlink). Only  */
        /* Windows has root relative symbolic links.             */
        if (unlikely(!ALLOC_OS_STRI(destination,
                      (memSizeType) DRIVE_PREFIX_LENGTH + substituteNameLength))) {
          errno = EACCES;
          result = NULL;
        } else {
          memcpy(destination, osSymlinkPath, DRIVE_PREFIX_LENGTH * sizeof(wchar_t));
          memcpy(&destination[DRIVE_PREFIX_LENGTH],
                 substituteName, substituteNameByteLen);
          destination[DRIVE_PREFIX_LENGTH + substituteNameLength] = '\0';
          result = winFollowSymlink(destination, numberOfFollowsAllowed - 1);
          if (result != destination) {
            FREE_OS_STRI(destination);
          } /* if */
        } /* if */
      } else if (unlikely(!ALLOC_OS_STRI(destination,
                           (memSizeType) substituteNameLength))) {
        errno = EACCES;
        result = NULL;
      } else {
        memcpy(destination, substituteName, substituteNameByteLen);
        /* Overwrite the substitute prefix \??\      */
        /* with the extended length path prefix \\?\ */
        memcpy(destination, PATH_PREFIX, PREFIX_LEN * sizeof(wchar_t));
        destination[substituteNameLength] = '\0';
        result = winFollowSymlink(destination, numberOfFollowsAllowed - 1);
        if (result != destination) {
          FREE_OS_STRI(destination);
        } /* if */
      } /* if */
    } else {
      /* A relative substitute name is relative to the    */
      /* directory of the symlink (and not relative to    */
      /* the current working directory). The relative     */
      /* substitute name is concatenated to the directory */
      /* of the given osSymlinkPath (=directory of the    */
      /* symlink).                                        */
      lastBackslashPos = os_stri_strrchr(osSymlinkPath, (wchar_t) '\\');
      if (unlikely(lastBackslashPos == NULL)) {
        logError(printf("followSymlinkRecursive(\"%ls\", ...): "
                        "Absolute path does not contain a backslash.\n",
                        osSymlinkPath););
        errno = EACCES;
        result = NULL;
      } else {
        directoryPathLength = (memSizeType) (lastBackslashPos - osSymlinkPath) + 1;
        if (unlikely(!ALLOC_OS_STRI(destination,
                      directoryPathLength + substituteNameLength))) {
          errno = EACCES;
          result = NULL;
        } else {
          memcpy(destination, osSymlinkPath, directoryPathLength * sizeof(wchar_t));
          memcpy(&destination[directoryPathLength],
                 substituteName, substituteNameByteLen);
          destination[directoryPathLength + substituteNameLength] = '\0';
          result = winFollowSymlink(destination, numberOfFollowsAllowed - 1);
          if (result != destination) {
            FREE_OS_STRI(destination);
          } /* if */
        } /* if */
      } /* if */
    } /* if */
    logFunction(printf("followSymlinkRecursive --> \"%ls\"\n", result););
    return result;
  } /* followSymlinkRecursive */



const wchar_t *winFollowSymlink (const wchar_t *path, int numberOfFollowsAllowed)

  {
    DWORD fileAttributes;
    HANDLE fileHandle;
    union info_t {
      char buffer[100]; /* Arbitrary buffer size (must be >= 28) */
      REPARSE_DATA_BUFFER7 reparseData;
    } info;
    memSizeType dataBufferHeadLength;
    memSizeType dataBufferLength;
    REPARSE_DATA_BUFFER7 *reparseData;
    DWORD bytesReturned;
    DWORD lastError;
    WCHAR *pathBuffer;
    const wchar_t *result;

  /* winFollowSymlink */
    logFunction(printf("winFollowSymlink(\"%ls\", %d)\n",
                       path, numberOfFollowsAllowed););
    if (numberOfFollowsAllowed == 0) {
#ifdef ELOOP
      errno = ELOOP;
#else
      errno = ENOENT;
#endif
      result = NULL;
    } else if (unlikely((fileAttributes = GetFileAttributesW(path)) ==
                        INVALID_FILE_ATTRIBUTES)) {
      logError(printf("winFollowSymlink(\"%ls\", ...): "
                      "GetFileAttributesW(\"%ls\") failed:\n"
                      "GetLastError=" FMT_U32 "\n",
                      path, path, (uint32Type) GetLastError()););
      errno = ENOENT;
      result = NULL;
    } else {
      if ((fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
        fileHandle = CreateFileW(path, 0,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT |
                                 FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (unlikely(fileHandle == INVALID_HANDLE_VALUE)) {
          logError(printf("winFollowSymlink(\"%ls\", ...): "
                          "CreateFileW(\"" FMT_S_OS "\", *) failed:\n"
                          "lastError=" FMT_U32 "\n",
                          path, path, (uint32Type) GetLastError()););
          errno = ENOENT;
          result = NULL;
        } else {
          if (unlikely(DeviceIoControl(fileHandle,
                                       FSCTL_GET_REPARSE_POINT,
                                       NULL, 0, info.buffer, sizeof(info),
                                       &bytesReturned, NULL) == 0)) {
            lastError = GetLastError();
            if (lastError != ERROR_MORE_DATA) {
              logError(printf("winFollowSymlink(\"%ls\", ...): "
                              "DeviceIoControl(" FMT_U_MEM ", ...) failed:\n"
                              "lastError=" FMT_U32 "%s\n",
                              path, (memSizeType) fileHandle,
                              (uint32Type) lastError,
                              lastError == ERROR_NOT_A_REPARSE_POINT ?
                                  " (ERROR_NOT_A_REPARSE_POINT)" : ""););
              errno = EACCES;
              result = NULL;
            } else if (unlikely(info.reparseData.ReparseTag !=
                                IO_REPARSE_TAG_SYMLINK &&
                                info.reparseData.ReparseTag !=
                                IO_REPARSE_TAG_MOUNT_POINT)) {
              logError(printf("winFollowSymlink(\"%ls\", ...): "
                              "Unexpected ReparseTag: 0x" FMT_X32 "\n",
                              path,
                              (uint32Type) info.reparseData.ReparseTag););
              errno = EACCES;
              result = NULL;
            } else {
              dataBufferHeadLength = (memSizeType)
                  ((char *) &info.reparseData.Destination -
                   (char *) &info.reparseData);
              dataBufferLength = dataBufferHeadLength +
                  info.reparseData.ReparseDataLength;
              reparseData = (REPARSE_DATA_BUFFER7 *) malloc(dataBufferLength);
              if (unlikely(reparseData == NULL)) {
                errno = EACCES;
                result = NULL;
              } else {
                if (unlikely(DeviceIoControl(fileHandle,
                                             FSCTL_GET_REPARSE_POINT, NULL, 0,
                                             reparseData, (DWORD) dataBufferLength,
                                             &bytesReturned, NULL) == 0)) {
                  logError(printf("winFollowSymlink(\"%ls\", ...): "
                                  "DeviceIoControl(" FMT_U_MEM ", ...) failed:\n"
                                  "lastError=" FMT_U32 "%\n",
                                  path, (memSizeType) fileHandle,
                                  (uint32Type) GetLastError()););
                  errno = EACCES;
                  result = NULL;
                } else {
                  if (reparseData->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
                    pathBuffer = reparseData->Destination.Data.SymbolicLink.PathBuffer;
                  } else if (reparseData->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
                    pathBuffer = reparseData->Destination.Data.MountPoint.PathBuffer;
                  } else {
                    logError(printf("winFollowSymlink(\"%ls\", ...): "
                                    "Unexpected ReparseTag: 0x" FMT_X32 "\n",
                                    path,
                                    (uint32Type) reparseData->ReparseTag););
                    pathBuffer = NULL;
                    errno = EACCES;
                    result = NULL;
                  } /* if */
                  if (likely(pathBuffer != NULL)) {
                    result = followSymlinkRecursive(path,
                        &pathBuffer[reparseData->Destination.SubstituteNameOffset /
                            sizeof(wchar_t)],
                        reparseData->Destination.SubstituteNameLength,
                        numberOfFollowsAllowed);
                  } /* if */
                } /* if */
                free(reparseData);
              } /* if */
            } /* if */
          } else {
            if (info.reparseData.ReparseTag == IO_REPARSE_TAG_SYMLINK) {
              pathBuffer = info.reparseData.Destination.Data.SymbolicLink.PathBuffer;
            } else if (info.reparseData.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
              pathBuffer = info.reparseData.Destination.Data.MountPoint.PathBuffer;
            } else {
              logError(printf("winFollowSymlink(\"%ls\", ...): "
                              "Unexpected ReparseTag: 0x" FMT_X32 "\n",
                              path,
                              (uint32Type) info.reparseData.ReparseTag););
              pathBuffer = NULL;
              errno = EACCES;
              result = NULL;
            } /* if */
            if (likely(pathBuffer != NULL)) {
              result = followSymlinkRecursive(path,
                  &pathBuffer[info.reparseData.Destination.SubstituteNameOffset /
                      sizeof(wchar_t)],
                  info.reparseData.Destination.SubstituteNameLength,
                  numberOfFollowsAllowed);
            } /* if */
          } /* if */
          CloseHandle(fileHandle);
        } /* if */
      } else {
        result = path;
      } /* if */
    } /* if */
    logFunction(printf("winFollowSymlink --> \"%ls\"\n", result););
    return result;
  } /* winFollowSymlink */



#ifdef DEFINE_WIN_SYMLINK
static void createSymlink (const wchar_t *osSymlinkPath,
    const wchar_t *sourceSymlinkPath, const wchar_t *substituteName,
    USHORT substituteNameByteLen, errInfoType *err_info)

  {
    USHORT substituteNameLength;
    wchar_t *destination;

  /* createSymlink */
    logFunction(printf("createSymlink(\"%ls\", \"%.*ls\", %hu, %d)\n",
                       osSymlinkPath,
                       (int) (substituteNameByteLen / sizeof(wchar_t)),
                       substituteName, substituteNameByteLen, *err_info););
    substituteNameLength = substituteNameByteLen / sizeof(wchar_t);
    if (substituteNameLength >= 1 && substituteName[0] == (wchar_t) '\\') {
      /* An absolute substitute name starts with \??\c:\    */
      /* (assuming that the drive letter of the path is c). */
      /* Our check allows also variants of this prefix.     */
      if (substituteNameLength < 7 ||
          substituteName[2] != (wchar_t) '?'  ||
          substituteName[3] != (wchar_t) '\\' ||
          substituteName[5] != (wchar_t) ':'  ||
          substituteName[6] != (wchar_t) '\\') {
        /* This is a root relative symbolic link. It is relative */
        /* to the drive of the symlink (and not relative to the  */
        /* current working drive). A root relative symbolic link */
        /* is converted to an absolute link. The root relative   */
        /* substitute name is concatenated to the drive taken    */
        /* from sourceSymlinkPath (=path of the source symlink). */
        /* Only Windows has root relative symbolic links.        */
        if (unlikely(!ALLOC_OS_STRI(destination,
                      (memSizeType) DRIVE_NAME_LENGTH + substituteNameLength))) {
          *err_info = MEMORY_ERROR;
        } else {
          if (osSymlinkPath[DRIVE_LETTER_INDEX] ==
              sourceSymlinkPath[DRIVE_LETTER_INDEX]) {
            memcpy(destination, substituteName, substituteNameByteLen);
            destination[substituteNameLength] = '\0';
          } else {
            memcpy(destination, &sourceSymlinkPath[DRIVE_LETTER_INDEX],
                   DRIVE_NAME_LENGTH * sizeof(wchar_t));
            memcpy(&destination[DRIVE_NAME_LENGTH],
                   substituteName, substituteNameByteLen);
            destination[DRIVE_NAME_LENGTH + substituteNameLength] = '\0';
          } /* if */
          if (unlikely(wSymlink(destination, osSymlinkPath) != 0)) {
            *err_info = FILE_ERROR;
          } /* if */
          FREE_OS_STRI(destination);
        } /* if */
      } else if (unlikely(!ALLOC_OS_STRI(destination,
                           (memSizeType) substituteNameLength - PREFIX_LEN))) {
        *err_info = MEMORY_ERROR;
      } else {
        /* Omit the substitute prefix \??\ from the destination path. */
        memcpy(destination, &substituteName[PREFIX_LEN],
               substituteNameByteLen - PREFIX_LEN * sizeof(wchar_t));
        destination[substituteNameLength - PREFIX_LEN] = '\0';
        if (unlikely(wSymlink(destination, osSymlinkPath) != 0)) {
          *err_info = FILE_ERROR;
        } /* if */
        FREE_OS_STRI(destination);
      } /* if */
    } else {
      /* Relative substitute name */
      if (unlikely(!ALLOC_OS_STRI(destination,
                    (memSizeType) substituteNameLength))) {
        *err_info = MEMORY_ERROR;
      } else {
        memcpy(destination, substituteName, substituteNameByteLen);
        destination[substituteNameLength] = '\0';
        if (unlikely(wSymlink(destination, osSymlinkPath) != 0)) {
          *err_info = FILE_ERROR;
        } /* if */
        FREE_OS_STRI(destination);
      } /* if */
    } /* if */
    logFunction(printf("createSymlink --> (err_info=%d)\n", *err_info););
  } /* createSymlink */



void winCopySymlink (const const_os_striType sourcePath,
    const const_os_striType destPath, errInfoType *err_info)

  {
    HANDLE fileHandle;
    union info_t {
      char buffer[100]; /* Arbitrary buffer size (must be >= 28) */
      REPARSE_DATA_BUFFER7 reparseData;
    } info;
    memSizeType dataBufferHeadLength;
    memSizeType dataBufferLength;
    REPARSE_DATA_BUFFER7 *reparseData;
    DWORD bytesReturned;
    DWORD lastError;
    WCHAR *pathBuffer;

  /* winCopySymlink */
    logFunction(printf("winCopySymlink(\"%ls\", \"%ls\", %d)\n",
                       sourcePath, destPath, *err_info););
    fileHandle = CreateFileW(sourcePath, 0,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT |
                             FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (unlikely(fileHandle == INVALID_HANDLE_VALUE)) {
      logError(printf("winCopySymlink(\"%ls\", \"%ls\", %d): "
                      "CreateFileW(\"" FMT_S_OS "\", *) failed:\n"
                      "lastError=" FMT_U32 "\n",
                      sourcePath, destPath, *err_info, sourcePath,
                      (uint32Type) GetLastError()););
      *err_info = FILE_ERROR;
    } else {
      if (unlikely(DeviceIoControl(fileHandle,
                                   FSCTL_GET_REPARSE_POINT,
                                   NULL, 0, info.buffer, sizeof(info),
                                   &bytesReturned, NULL) == 0)) {
        lastError = GetLastError();
        if (lastError != ERROR_MORE_DATA) {
          logError(printf("winCopySymlink(\"%ls\", \"%ls\", %d): "
                          "DeviceIoControl(" FMT_U_MEM ", ...) failed:\n"
                          "lastError=" FMT_U32 "%s\n",
                          sourcePath, destPath, *err_info,
                          (memSizeType) fileHandle, (uint32Type) lastError,
                          lastError == ERROR_NOT_A_REPARSE_POINT ?
                              " (ERROR_NOT_A_REPARSE_POINT)" : ""););
          *err_info = FILE_ERROR;
        } else if (unlikely(info.reparseData.ReparseTag !=
                            IO_REPARSE_TAG_SYMLINK &&
                            info.reparseData.ReparseTag !=
                            IO_REPARSE_TAG_MOUNT_POINT)) {
          logError(printf("winCopySymlink(\"%ls\", \"%ls\", %d): "
                          "Unexpected ReparseTag: 0x" FMT_X32 "\n",
                          sourcePath, destPath, *err_info,
                          (uint32Type) info.reparseData.ReparseTag););
          *err_info = FILE_ERROR;
        } else {
          dataBufferHeadLength = (memSizeType)
              ((char *) &info.reparseData.Destination -
               (char *) &info.reparseData);
          dataBufferLength = dataBufferHeadLength +
              info.reparseData.ReparseDataLength;
          reparseData = (REPARSE_DATA_BUFFER7 *) malloc(dataBufferLength);
          if (unlikely(reparseData == NULL)) {
            *err_info = FILE_ERROR;
          } else {
            if (unlikely(DeviceIoControl(fileHandle,
                                         FSCTL_GET_REPARSE_POINT, NULL, 0,
                                         reparseData, (DWORD) dataBufferLength,
                                         &bytesReturned, NULL) == 0)) {
              logError(printf("winCopySymlink(\"%ls\", \"%ls\", %d): "
                              "DeviceIoControl(" FMT_U_MEM ", ...) failed:\n"
                              "lastError=" FMT_U32 "%\n",
                              sourcePath, destPath, *err_info,
                              (memSizeType) fileHandle,
                              (uint32Type) GetLastError()););
              *err_info = FILE_ERROR;
            } else {
              if (reparseData->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
                pathBuffer = reparseData->Destination.Data.SymbolicLink.PathBuffer;
              } else if (reparseData->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
                pathBuffer = reparseData->Destination.Data.MountPoint.PathBuffer;
              } else {
                logError(printf("winCopySymlink(\"%ls\", (\"%ls\", %d): "
                                "Unexpected ReparseTag: 0x" FMT_X32 "\n",
                                sourcePath, destPath, *err_info,
                                (uint32Type) reparseData->ReparseTag););
                pathBuffer = NULL;
                *err_info = FILE_ERROR;
              } /* if */
              if (likely(pathBuffer != NULL)) {
                createSymlink(destPath, sourcePath,
                    &pathBuffer[reparseData->Destination.SubstituteNameOffset /
                        sizeof(wchar_t)],
                    reparseData->Destination.SubstituteNameLength,
                    err_info);
              } /* if */
            } /* if */
            free(reparseData);
          } /* if */
        } /* if */
      } else {
        if (info.reparseData.ReparseTag == IO_REPARSE_TAG_SYMLINK) {
          pathBuffer = info.reparseData.Destination.Data.SymbolicLink.PathBuffer;
        } else if (info.reparseData.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
          pathBuffer = info.reparseData.Destination.Data.MountPoint.PathBuffer;
        } else {
          logError(printf("winCopySymlink(\"%ls\", \"%ls\", %d): "
                          "Unexpected ReparseTag: 0x" FMT_X32 "\n",
                          sourcePath, destPath, *err_info,
                          (uint32Type) info.reparseData.ReparseTag););
          pathBuffer = NULL;
          *err_info = FILE_ERROR;
        } /* if */
        if (likely(pathBuffer != NULL)) {
          createSymlink(destPath, sourcePath,
              &pathBuffer[info.reparseData.Destination.SubstituteNameOffset /
                  sizeof(wchar_t)],
              info.reparseData.Destination.SubstituteNameLength,
              err_info);
        } /* if */
      } /* if */
      CloseHandle(fileHandle);
    } /* if */
    logFunction(printf("winCopySymlink(\"%ls\", \"%ls\", %d) -->\n",
                       sourcePath, destPath, *err_info););
  } /* winCopySymlink */
#endif
#endif



static boolType setupWellKnownSids (void)

  { /* setupWellKnownSids */
    if (unlikely(administratorSid == NULL)) {
      AllocateAndInitializeSid(&ntAuthority, 2,
                               SECURITY_BUILTIN_DOMAIN_RID,
                               DOMAIN_ALIAS_RID_ADMINS,
                               0, 0, 0, 0, 0, 0,
                               &administratorSid);
    } /* if */
    if (unlikely(worldSid == NULL)) {
      AllocateAndInitializeSid(&worldSidAuthority, 1,
                               SECURITY_WORLD_RID,
                               0, 0, 0, 0, 0, 0, 0,
                               &worldSid);
    } /* if */
    return administratorSid != NULL && worldSid != NULL;
  } /* setupWellKnownSids */



static striType getNameFromSid (PSID sid, errInfoType *err_info)

  {
    LPWSTR AcctName;
    DWORD sizeAcctName = 0;
    LPWSTR DomainName;
    DWORD sizeDomainName = 0;
    SID_NAME_USE eUse = SidTypeUnknown;
    striType name;

  /* getNameFromSid */
    if (unlikely(!setupWellKnownSids())) {
      *err_info = MEMORY_ERROR;
      name = NULL;
    } else if (memcmp(sid, administratorSid, sizeof(SID)) == 0) {
      name = cstri_to_stri("root");
      if (unlikely(name == NULL)) {
        *err_info = MEMORY_ERROR;
      } /* if */
    } else if (memcmp(sid, worldSid, sizeof(SID)) == 0) {
      name = cstri_to_stri("world");
      if (unlikely(name == NULL)) {
        *err_info = MEMORY_ERROR;
      } /* if */
    } else {
      LookupAccountSidW(NULL, sid,
                        NULL, (LPDWORD) &sizeAcctName,
                        NULL, (LPDWORD) &sizeDomainName, &eUse);
      AcctName = (LPWSTR) GlobalAlloc(GMEM_FIXED, sizeAcctName * sizeof(os_charType));
      DomainName = (LPWSTR) GlobalAlloc(GMEM_FIXED, sizeDomainName * sizeof(os_charType));
      if (unlikely(AcctName == NULL || DomainName == NULL)) {
        logError(printf("getNameFromSid: GlobalAlloc() failed:\n"
                        "lastError=" FMT_U32 "\n",
                        (uint32Type) GetLastError()););
        if (AcctName != NULL) {
          GlobalFree(AcctName);
        } /* if */
        *err_info = MEMORY_ERROR;
        name = NULL;
      } else {
        if (unlikely(LookupAccountSidW(NULL, sid,
                                       AcctName, (LPDWORD) &sizeAcctName,
                                       DomainName, (LPDWORD) &sizeDomainName,
                                       &eUse) == FALSE)) {
          logError(printf("getNameFromSid: LookupAccountSidW() failed:\n"
                          "lastError=" FMT_U32 "\n",
                          (uint32Type) GetLastError()););
          *err_info = FILE_ERROR;
          name = NULL;
        } else {
          name = os_stri_to_stri(AcctName, err_info);
        } /* if */
        GlobalFree(AcctName);
        GlobalFree(DomainName);
      } /* if */
    } /* if */
    return name;
  } /* getNameFromSid */



static PSID getSidFromName (const const_striType name, errInfoType *err_info)

  {
    os_striType accountName;
    DWORD numberOfBytesForSid = 0;
    DWORD numberOfCharsForDomainName = 0;
    os_striType domainName;
    SID_NAME_USE sidNameUse = SidTypeInvalid;
    PSID sid = NULL;

  /* getSidFromName */
    logFunction(printf("getSidFromName(\"%s\", %d)\n",
                       striAsUnquotedCStri(name), *err_info););
    accountName = stri_to_os_stri(name, err_info);
    if (likely(accountName != NULL)) {
      if (unlikely(!setupWellKnownSids())) {
        *err_info = MEMORY_ERROR;
      } else if (memcmp(accountName, L"root\0", 5 * sizeof(os_charType)) == 0) {
        numberOfBytesForSid = GetLengthSid(administratorSid);
        sid = (PSID) malloc(numberOfBytesForSid);
        if (unlikely(sid == NULL)) {
          *err_info = MEMORY_ERROR;
        } else {
          memcpy(sid, administratorSid, numberOfBytesForSid);
        } /* if */
      } else if (memcmp(accountName, L"world\0", 6 * sizeof(os_charType)) == 0) {
        numberOfBytesForSid = GetLengthSid(worldSid);
        sid = (PSID) malloc(numberOfBytesForSid);
        if (unlikely(sid == NULL)) {
          *err_info = MEMORY_ERROR;
        } else {
          memcpy(sid, worldSid, numberOfBytesForSid);
        } /* if */
      } else {
        LookupAccountNameW(NULL, accountName, NULL, &numberOfBytesForSid,
                           NULL, &numberOfCharsForDomainName, &sidNameUse);
        sid = (PSID) malloc(numberOfBytesForSid);
        if (unlikely(sid == NULL)) {
          *err_info = MEMORY_ERROR;
        } else if (unlikely(!ALLOC_OS_STRI(domainName, numberOfCharsForDomainName))) {
          free(sid);
          *err_info = MEMORY_ERROR;
          sid = NULL;
        } else {
          if (unlikely(LookupAccountNameW(NULL, accountName, sid, &numberOfBytesForSid,
                                          domainName, &numberOfCharsForDomainName, &sidNameUse) == 0)) {
            logError(printf("getSidFromName: LookupAccountNameW failed:\n"
                            "lastError=" FMT_U32 "\n",
                            (uint32Type) GetLastError()););
            free(sid);
            *err_info = RANGE_ERROR;
            sid = NULL;
          } /* if */
          FREE_OS_STRI(domainName);
        } /* if */
      } /* if */
      os_stri_free(accountName);
    } /* if */
    return sid;
  } /* getSidFromName */



/**
 *  Determine the name of the group (GID) to which a file belongs.
 *  The function follows symbolic links.
 *  @return the name of the file group.
 *  @exception MEMORY_ERROR Not enough memory to convert 'filePath'
 *             to the system path type.
 *  @exception RANGE_ERROR 'filePath' does not use the standard path
 *             representation or it cannot be converted to the system
 *             path type.
 *  @exception FILE_ERROR The file described with 'filePath' does not
 *             exist, or a system function returns an error.
 */
striType cmdGetGroup (const const_striType filePath)

  {
    os_striType os_path;
    int path_info = PATH_IS_NORMAL;
    errInfoType err_info = OKAY_NO_ERROR;
    HANDLE fileHandle;
    DWORD returnCode;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PSID pSidGroup = NULL;
    striType group;

  /* cmdGetGroup */
    logFunction(printf("cmdGetGroup(\"%s\")", striAsUnquotedCStri(filePath));
                fflush(stdout););
    os_path = cp_to_os_path(filePath, &path_info, &err_info);
    if (unlikely(os_path == NULL)) {
#if MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        /* The emulated root is in the group root. Do not raise an exception. */
        err_info = OKAY_NO_ERROR;
        group = cstri_to_stri("root");
        if (unlikely(group == NULL)) {
          err_info = MEMORY_ERROR;
          group = NULL;
        } /* if */
      } else
#endif
      {
        logError(printf("cmdGetGroup: cp_to_os_path(\"%s\", *, *) failed:\n"
                        "path_info=%d, err_info=%d\n",
                        striAsUnquotedCStri(filePath), path_info, err_info););
        group = NULL;
      }
    } else {
      fileHandle = CreateFileW(os_path, READ_CONTROL, 0,
                               NULL, OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS, NULL);
      if (unlikely(fileHandle == INVALID_HANDLE_VALUE)) {
        logError(printf("cmdGetGroup(\"%s\"): "
                        "CreateFileW(\"" FMT_S_OS "\", *) failed:\n"
                        "lastError=" FMT_U32 "\n",
                        striAsUnquotedCStri(filePath),
                        os_path, (uint32Type) GetLastError()););
        err_info = FILE_ERROR;
        group = NULL;
      } else {
        returnCode = GetSecurityInfo(fileHandle, SE_FILE_OBJECT,
                                     GROUP_SECURITY_INFORMATION, NULL,
                                     &pSidGroup, NULL, NULL, &pSD);
        if (unlikely(returnCode != ERROR_SUCCESS)) {
          logError(printf("cmdGetGroup(\"%s\"): "
                          "GetSecurityInfo(" FMT_U_MEM ", ...) failed:\n"
                          "returnCode=" FMT_U32 "\n",
                          striAsUnquotedCStri(filePath),
                          (memSizeType) fileHandle,
                          (uint32Type) returnCode););
          err_info = FILE_ERROR;
          group = NULL;
        } else {
          group = getNameFromSid(pSidGroup, &err_info);
          /* The SID referenced by pSidOwner is located */
          /* inside of the PSECURITY_DESCRIPTOR pSD.    */
          /* Therefore it is freed together with pSD.   */
          LocalFree(pSD);
        } /* if */
        CloseHandle(fileHandle);
      } /* if */
      os_stri_free(os_path);
    } /* if */
    if (unlikely(group == NULL)) {
      raise_error(err_info);
    } /* if */
    logFunctionResult(printf("\"%s\"\n", striAsUnquotedCStri(group)););
    return group;
  } /* cmdGetGroup */



/**
 *  Determine the name of the group (GID) to which a symbolic link belongs.
 *  The function only works for symbolic links and does not follow the
 *  symbolic link.
 *  @return the name of the file group.
 *  @exception MEMORY_ERROR Not enough memory to convert 'filePath'
 *             to the system path type.
 *  @exception RANGE_ERROR 'filePath' does not use the standard path
 *             representation or it cannot be converted to the system
 *             path type.
 *  @exception FILE_ERROR The file described with 'filePath' does not
 *             exist, or it is not a symbolic link, or a system function
 *             returns an error.
 */
striType cmdGetGroupOfSymlink (const const_striType filePath)

  {
    os_striType os_path;
    int path_info = PATH_IS_NORMAL;
    errInfoType err_info = OKAY_NO_ERROR;
    DWORD fileAttributes;
    DWORD returnCode;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PSID pSidGroup = NULL;
    striType group;

  /* cmdGetGroupOfSymlink */
    logFunction(printf("cmdGetGroupOfSymlink(\"%s\")", striAsUnquotedCStri(filePath));
                fflush(stdout););
    os_path = cp_to_os_path(filePath, &path_info, &err_info);
    if (unlikely(os_path == NULL)) {
#if MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        /* The emulated root is not a symbolic link. */
        logError(printf("cmdGetGroupOfSymlink(\"%s\"): "
                        "The emulated root is not a symbolic link.\n",
                        striAsUnquotedCStri(filePath)););
        err_info = FILE_ERROR;
        group = NULL;
      } else
#endif
      {
        logError(printf("cmdGetGroupOfSymlink: cp_to_os_path(\"%s\", *, *) failed:\n"
                        "path_info=%d, err_info=%d\n",
                        striAsUnquotedCStri(filePath), path_info, err_info););
        group = NULL;
      }
    } else {
      if (unlikely((fileAttributes = GetFileAttributesW(os_path)) ==
                   INVALID_FILE_ATTRIBUTES)) {
        logError(printf("cmdGetGroupOfSymlink(\"%s\"): "
                        "GetFileAttributesW(\"" FMT_S_OS "\") failed:\n"
                        "lastError=" FMT_U32 "\n",
                        striAsUnquotedCStri(filePath), os_path,
                        (uint32Type) GetLastError()););
        err_info = FILE_ERROR;
        group = NULL;
      } else if ((fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
        logError(printf("cmdGetGroupOfSymlink(\"%s\"): "
                        "The file \"" FMT_S_OS "\" is not a symbolic link.\n",
                        striAsUnquotedCStri(filePath), os_path););
        err_info = FILE_ERROR;
        group = NULL;
      } else {
        returnCode = GetNamedSecurityInfoW(os_path, SE_FILE_OBJECT,
                                           GROUP_SECURITY_INFORMATION,
                                           NULL, &pSidGroup, NULL, NULL,
                                           &pSD);
        if (unlikely(returnCode != ERROR_SUCCESS)) {
          logError(printf("cmdGetGroupOfSymlink(\"%s\"): "
                          "GetNamedSecurityInfoW(\"" FMT_S_OS "\", ...) failed:\n"
                          "returnCode=" FMT_U32 "\n",
                          striAsUnquotedCStri(filePath),
                          os_path, (uint32Type) returnCode););
          err_info = FILE_ERROR;
          group = NULL;
        } else {
          group = getNameFromSid(pSidGroup, &err_info);
          /* The SID referenced by pSidOwner is located */
          /* inside of the PSECURITY_DESCRIPTOR pSD.    */
          /* Therefore it is freed together with pSD.   */
          LocalFree(pSD);
        } /* if */
      } /* if */
      os_stri_free(os_path);
    } /* if */
    if (unlikely(group == NULL)) {
      raise_error(err_info);
    } /* if */
    logFunctionResult(printf("\"%s\"\n", striAsUnquotedCStri(group)););
    return group;
  } /* cmdGetGroupOfSymlink */



/**
 *  Determine the name of the owner (UID) of a file.
 *  The function follows symbolic links.
 *  @return the name of the file owner.
 *  @exception MEMORY_ERROR Not enough memory to convert 'filePath'
 *             to the system path type.
 *  @exception RANGE_ERROR 'filePath' does not use the standard path
 *             representation or it cannot be converted to the system
 *             path type.
 *  @exception FILE_ERROR The file described with 'filePath' does not
 *             exist, or a system function returns an error.
 */
striType cmdGetOwner (const const_striType filePath)

  {
    os_striType os_path;
    int path_info = PATH_IS_NORMAL;
    errInfoType err_info = OKAY_NO_ERROR;
    HANDLE fileHandle;
    DWORD returnCode;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PSID pSidOwner = NULL;
    striType owner;

  /* cmdGetOwner */
    logFunction(printf("cmdGetOwner(\"%s\")", striAsUnquotedCStri(filePath));
                fflush(stdout););
    os_path = cp_to_os_path(filePath, &path_info, &err_info);
    if (unlikely(os_path == NULL)) {
#if MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        /* The emulated root is owned by root. Do not raise an exception. */
        err_info = OKAY_NO_ERROR;
        owner = cstri_to_stri("root");
        if (unlikely(owner == NULL)) {
          err_info = MEMORY_ERROR;
          owner = NULL;
        } /* if */
      } else
#endif
      {
        logError(printf("cmdGetOwner: cp_to_os_path(\"%s\", *, *) failed:\n"
                        "path_info=%d, err_info=%d\n",
                        striAsUnquotedCStri(filePath), path_info, err_info););
        owner = NULL;
      }
    } else {
      fileHandle = CreateFileW(os_path, READ_CONTROL, 0,
                               NULL, OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS, NULL);
      if (unlikely(fileHandle == INVALID_HANDLE_VALUE)) {
        logError(printf("cmdGetOwner(\"%s\"): "
                        "CreateFileW(\"" FMT_S_OS "\", *) failed:\n"
                        "lastError=" FMT_U32 "\n",
                        striAsUnquotedCStri(filePath),
                        os_path, (uint32Type) GetLastError()););
        err_info = FILE_ERROR;
        owner = NULL;
      } else {
        returnCode = GetSecurityInfo(fileHandle, SE_FILE_OBJECT,
                                     OWNER_SECURITY_INFORMATION,
                                     &pSidOwner, NULL, NULL, NULL, &pSD);
        if (unlikely(returnCode != ERROR_SUCCESS)) {
          logError(printf("cmdGetOwner(\"%s\"): "
                          "GetSecurityInfo(" FMT_U_MEM ", ...) failed:\n"
                          "returnCode=" FMT_U32 "\n",
                          striAsUnquotedCStri(filePath),
                          (memSizeType) fileHandle,
                          (uint32Type) returnCode););
          err_info = FILE_ERROR;
          owner = NULL;
        } else {
          owner = getNameFromSid(pSidOwner, &err_info);
          /* The SID referenced by pSidOwner is located */
          /* inside of the PSECURITY_DESCRIPTOR pSD.    */
          /* Therefore it is freed together with pSD.   */
          LocalFree(pSD);
        } /* if */
        CloseHandle(fileHandle);
      } /* if */
      os_stri_free(os_path);
    } /* if */
    if (unlikely(owner == NULL)) {
      raise_error(err_info);
    } /* if */
    logFunctionResult(printf("\"%s\"\n", striAsUnquotedCStri(owner)););
    return owner;
  } /* cmdGetOwner */



/**
 *  Determine the name of the owner (UID) of a symbolic link.
 *  The function only works for symbolic links and does not follow the
 *  symbolic link.
 *  @return the name of the file owner.
 *  @exception MEMORY_ERROR Not enough memory to convert 'filePath'
 *             to the system path type.
 *  @exception RANGE_ERROR 'filePath' does not use the standard path
 *             representation or it cannot be converted to the system
 *             path type.
 *  @exception FILE_ERROR The file described with 'filePath' does not
 *             exist, or it is not a symbolic link, or a system function
 *             returns an error.
 */
striType cmdGetOwnerOfSymlink (const const_striType filePath)

  {
    os_striType os_path;
    int path_info = PATH_IS_NORMAL;
    errInfoType err_info = OKAY_NO_ERROR;
    DWORD fileAttributes;
    DWORD returnCode;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PSID pSidOwner = NULL;
    striType owner;

  /* cmdGetOwnerOfSymlink */
    logFunction(printf("cmdGetOwnerOfSymlink(\"%s\")", striAsUnquotedCStri(filePath));
                fflush(stdout););
    os_path = cp_to_os_path(filePath, &path_info, &err_info);
    if (unlikely(os_path == NULL)) {
#if MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        /* The emulated root is not a symbolic link. */
        logError(printf("cmdGetOwnerOfSymlink(\"%s\"): "
                        "The emulated root is not a symbolic link.\n",
                        striAsUnquotedCStri(filePath)););
        err_info = FILE_ERROR;
        owner = NULL;
      } else
#endif
      {
        logError(printf("cmdGetOwnerOfSymlink: cp_to_os_path(\"%s\", *, *) failed:\n"
                        "path_info=%d, err_info=%d\n",
                        striAsUnquotedCStri(filePath), path_info, err_info););
        owner = NULL;
      }
    } else {
      if (unlikely((fileAttributes = GetFileAttributesW(os_path)) ==
                   INVALID_FILE_ATTRIBUTES)) {
        logError(printf("cmdGetOwnerOfSymlink(\"%s\"): "
                        "GetFileAttributesW(\"" FMT_S_OS "\") failed:\n"
                        "lastError=" FMT_U32 "\n",
                        striAsUnquotedCStri(filePath), os_path,
                        (uint32Type) GetLastError()););
        err_info = FILE_ERROR;
        owner = NULL;
      } else if ((fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
        logError(printf("cmdGetOwnerOfSymlink(\"%s\"): "
                        "The file \"" FMT_S_OS "\" is not a symbolic link.\n",
                        striAsUnquotedCStri(filePath), os_path););
        err_info = FILE_ERROR;
        owner = NULL;
      } else {
        returnCode = GetNamedSecurityInfoW(os_path, SE_FILE_OBJECT,
                                           OWNER_SECURITY_INFORMATION,
                                           &pSidOwner, NULL, NULL, NULL,
                                           &pSD);
        if (unlikely(returnCode != ERROR_SUCCESS)) {
          logError(printf("cmdGetOwnerOfSymlink(\"%s\"): "
                          "GetNamedSecurityInfoW(\"" FMT_S_OS "\", ...) failed:\n"
                          "returnCode=" FMT_U32 "\n",
                          striAsUnquotedCStri(filePath),
                          os_path, (uint32Type) returnCode););
          err_info = FILE_ERROR;
          owner = NULL;
        } else {
          owner = getNameFromSid(pSidOwner, &err_info);
          /* The SID referenced by pSidOwner is located */
          /* inside of the PSECURITY_DESCRIPTOR pSD.    */
          /* Therefore it is freed together with pSD.   */
          LocalFree(pSD);
        } /* if */
      } /* if */
      os_stri_free(os_path);
    } /* if */
    if (unlikely(owner == NULL)) {
      raise_error(err_info);
    } /* if */
    logFunctionResult(printf("\"%s\"\n", striAsUnquotedCStri(owner)););
    return owner;
  } /* cmdGetOwnerOfSymlink */



/**
 *  Set the group of a file.
 *  The function follows symbolic links.
 *  @exception MEMORY_ERROR Not enough memory to convert 'filePath'
 *             to the system path type.
 *  @exception RANGE_ERROR 'filePath' does not use the standard path
 *             representation or it cannot be converted to the system
 *             path type.
 *  @exception FILE_ERROR The file described with 'filePath' does not
 *             exist, or a system function returns an error.
 */
void cmdSetGroup (const const_striType filePath, const const_striType group)

  {
    os_striType os_path;
    int path_info = PATH_IS_NORMAL;
    errInfoType err_info = OKAY_NO_ERROR;
    HANDLE fileHandle;
    DWORD returnCode;
    PSID pSidGroup;

  /* cmdSetGroup */
    logFunction(printf("cmdSetGroup(\"%s\", ", striAsUnquotedCStri(filePath));
                printf("\"%s\")\n", striAsUnquotedCStri(group)));
    os_path = cp_to_os_path(filePath, &path_info, &err_info);
    if (unlikely(os_path == NULL)) {
      logError(printf("cmdSetGroup: cp_to_os_path(\"%s\", *, *) failed:\n"
                      "path_info=%d, err_info=%d\n",
                      striAsUnquotedCStri(filePath), path_info, err_info););
    } else {
      fileHandle = CreateFileW(os_path, WRITE_OWNER, 0,
                               NULL, OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS, NULL);
      if (unlikely(fileHandle == INVALID_HANDLE_VALUE)) {
        logError(printf("cmdSetGroup(\"%s\"): "
                        "CreateFileW(\"" FMT_S_OS "\", *) failed:\n"
                        "lastError=" FMT_U32 "\n",
                        striAsUnquotedCStri(filePath),
                        os_path, (uint32Type) GetLastError()););
        err_info = FILE_ERROR;
      } else {
        pSidGroup = getSidFromName(group, &err_info);
        if (likely(pSidGroup != NULL)) {
          returnCode = SetSecurityInfo(fileHandle, SE_FILE_OBJECT,
                                       GROUP_SECURITY_INFORMATION,
                                       NULL, pSidGroup, NULL, NULL);
          if (unlikely(returnCode != ERROR_SUCCESS)) {
            logError(printf("cmdSetGroup(\"%s\", ",
                            striAsUnquotedCStri(filePath));
                     printf("\"%s\"): "
                            "SetSecurityInfo(" FMT_U_MEM ", ...) failed:\n"
                            "returnCode=" FMT_U32 "\n",
                            striAsUnquotedCStri(group),
                            (memSizeType) fileHandle,
                            (uint32Type) returnCode););
            err_info = FILE_ERROR;
          } /* if */
          free(pSidGroup);
        } /* if */
        CloseHandle(fileHandle);
      } /* if */
      os_stri_free(os_path);
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
  } /* cmdSetGroup */



/**
 *  Set the group of a symbolic link.
 *  The function only works for symbolic links and does not follow the
 *  symbolic link.
 *  @exception MEMORY_ERROR Not enough memory to convert 'filePath'
 *             to the system path type.
 *  @exception RANGE_ERROR 'filePath' does not use the standard path
 *             representation or it cannot be converted to the system
 *             path type.
 *  @exception FILE_ERROR The file described with 'filePath' does not
 *             exist, or it is not a symbolic link, or a system function
 *             returns an error.
 */
void cmdSetGroupOfSymlink (const const_striType filePath, const const_striType group)

  {
    os_striType os_path;
    int path_info = PATH_IS_NORMAL;
    errInfoType err_info = OKAY_NO_ERROR;
    DWORD fileAttributes;
    DWORD returnCode;
    PSID pSidGroup;

  /* cmdSetGroupOfSymlink */
    logFunction(printf("cmdSetGroupOfSymlink(\"%s\", ", striAsUnquotedCStri(filePath));
                printf("\"%s\")\n", striAsUnquotedCStri(group)));
    os_path = cp_to_os_path(filePath, &path_info, &err_info);
    if (unlikely(os_path == NULL)) {
#if MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        /* The emulated root is not a symbolic link. */
        logError(printf("cmdSetGroupOfSymlink(\"%s\"): "
                        "The emulated root is not a symbolic link.\n",
                        striAsUnquotedCStri(filePath)););
        err_info = FILE_ERROR;
      } else
#endif
      {
        logError(printf("cmdSetGroupOfSymlink: cp_to_os_path(\"%s\", *, *) failed:\n"
                        "path_info=%d, err_info=%d\n",
                        striAsUnquotedCStri(filePath), path_info, err_info););
      }
    } else {
      if (unlikely((fileAttributes = GetFileAttributesW(os_path)) ==
                   INVALID_FILE_ATTRIBUTES)) {
        logError(printf("cmdSetGroupOfSymlink(\"%s\", ...): "
                        "GetFileAttributesW(\"" FMT_S_OS "\") failed:\n"
                        "lastError=" FMT_U32 "\n",
                        striAsUnquotedCStri(filePath), os_path,
                        (uint32Type) GetLastError()););
        err_info = FILE_ERROR;
      } else if ((fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
        logError(printf("cmdSetGroupOfSymlink(\"%s\", ...): "
                        "The file \"" FMT_S_OS "\" is not a symbolic link.\n",
                        striAsUnquotedCStri(filePath), os_path););
        err_info = FILE_ERROR;
      } else {
        pSidGroup = getSidFromName(group, &err_info);
        if (likely(pSidGroup != NULL)) {
          returnCode = SetNamedSecurityInfoW(os_path, SE_FILE_OBJECT,
                                             GROUP_SECURITY_INFORMATION,
                                             NULL, pSidGroup, NULL, NULL);
          if (unlikely(returnCode != ERROR_SUCCESS)) {
            logError(printf("cmdSetGroupOfSymlink(\"%s\", ",
                            striAsUnquotedCStri(filePath));
                     printf("\"%s\"): "
                            "SetNamedSecurityInfoW(\"" FMT_S_OS "\", ...) failed:\n"
                            "returnCode=" FMT_U32 "\n",
                            striAsUnquotedCStri(group), os_path,
                            (uint32Type) returnCode););
            err_info = FILE_ERROR;
          } /* if */
          free(pSidGroup);
        } /* if */
      } /* if */
      os_stri_free(os_path);
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
  } /* cmdSetGroupOfSymlink */



/**
 *  Set the modification time of a symbolic link.
 *  The function only works for symbolic links and does not follow the
 *  symbolic link.
 *  @exception MEMORY_ERROR Not enough memory to convert 'filePath'
 *             to the system path type.
 *  @exception RANGE_ERROR 'filePath' does not use the standard path
 *             representation or it cannot be converted to the system
 *             path type.
 *  @exception RANGE_ERROR The time is invalid or cannot be
 *             converted to the system file time.
 *  @exception FILE_ERROR The file described with 'filePath' does not
 *             exist, or it is not a symbolic link, or a system function
 *             returns an error.
 */
void cmdSetMTimeOfSymlink (const const_striType filePath,
    intType year, intType month, intType day, intType hour,
    intType min, intType sec, intType micro_sec, intType time_zone)

  {
    const_os_striType os_path;
    int path_info;
    errInfoType err_info = OKAY_NO_ERROR;
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    timeStampType modificationTime;
    union {
      uint64Type nanosecs100; /*time since 1 Jan 1601 in 100ns units */
      FILETIME filetime;
    } modificationFileTime;
    HANDLE fileHandle;

  /* cmdSetMTimeOfSymlink */
    logFunction(printf("cmdSetMTimeOfSymlink(\"%s\", " F_D(04) "-" F_D(02) "-" F_D(02) " "
                       F_D(02) ":" F_D(02) ":" F_D(02) "." F_D(06) " " FMT_D ")\n",
                       striAsUnquotedCStri(filePath), year, month,
                       day, hour, min, sec, micro_sec, time_zone););
    os_path = cp_to_os_path(filePath, &path_info, &err_info);
    if (unlikely(os_path == NULL)) {
#if MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        /* The emulated root is not a symbolic link. */
        logError(printf("cmdSetMTimeOfSymlink(\"%s\"): "
                        "The emulated root is not a symbolic link.\n",
                        striAsUnquotedCStri(filePath)););
        err_info = FILE_ERROR;
      } else
#endif
      {
        logError(printf("cmdSetMTimeOfSymlink: cp_to_os_path(\"%s\", *, *) failed:\n"
                        "path_info=%d, err_info=%d\n",
                        striAsUnquotedCStri(filePath), path_info, err_info););
      }
    } else {
      if (unlikely(GetFileAttributesExW(os_path, GetFileExInfoStandard, &fileInfo) == 0)) {
        logError(printf("cmdSetMTimeOfSymlink(\"%s\", ...): "
                        "GetFileAttributesExW(\"" FMT_S_OS "\", ...) failed:\n"
                        "lastError=" FMT_U32 "\n",
                        striAsUnquotedCStri(filePath), os_path,
                        (uint32Type) GetLastError()););
        err_info = FILE_ERROR;
      } else if ((fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
        logError(printf("cmdSetMTimeOfSymlink(\"%s\", ...): "
                        "The file \"" FMT_S_OS "\" is not a symbolic link.\n",
                        striAsUnquotedCStri(filePath), os_path););
        err_info = FILE_ERROR;
      } else {
        modificationTime = timToTimestamp(year, month, day, hour, min, sec,
                                          time_zone);
        /* printf("modificationTime=" FMT_D64 "\n", modificationTime); */
        if (unlikely(modificationTime == TIMESTAMPTYPE_MIN ||
                     modificationTime < -SECONDS_FROM_1601_TO_1970 ||
                     modificationTime > UINT64TYPE_MAX / WINDOWS_TICK -
                                        SECONDS_FROM_1601_TO_1970)) {
          logError(printf("cmdSetMTimeOfSymlink: timToTimestamp("
                          F_D(04) "-" F_D(02) "-" F_D(02) " " F_D(02) ":"
                          F_D(02) ":" F_D(02) "." F_D(06) " " FMT_D ") failed "
                          "or the result does not fit into a FILETIME.\n",
                          year, month, day, hour, min, sec,
                          micro_sec, time_zone););
          err_info = RANGE_ERROR;
        } else {
          modificationFileTime.nanosecs100 = (uint64Type) (
              modificationTime + SECONDS_FROM_1601_TO_1970) * WINDOWS_TICK;
          /* printf("filetime=" FMT_U64 "\n", modificationFileTime.nanosecs100); */
          fileHandle = CreateFileW(os_path, FILE_WRITE_ATTRIBUTES, 0, NULL,
                                   OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT |
                                   FILE_FLAG_BACKUP_SEMANTICS, NULL);
          if (unlikely(fileHandle == INVALID_HANDLE_VALUE)) {
            logError(printf("cmdSetMTimeOfSymlink(\"%s\"): "
                            "CreateFileW(\"" FMT_S_OS "\", *) failed:\n"
                            "lastError=" FMT_U32 "\n",
                            striAsUnquotedCStri(filePath),
                            os_path, (uint32Type) GetLastError()););
            err_info = FILE_ERROR;
          } else {
            if (unlikely(SetFileTime(fileHandle,
                                     &fileInfo.ftCreationTime,
                                     &fileInfo.ftLastAccessTime,
                                     &modificationFileTime.filetime) == 0)) {
              logError(printf("cmdSetMTimeOfSymlink(\"%s\"): "
                              "SetFileTime(\"" FMT_S_OS "\", *) failed:\n"
                              "lastError=" FMT_U32 "\n",
                              striAsUnquotedCStri(filePath),
                              os_path, (uint32Type) GetLastError()););
              err_info = FILE_ERROR;
            } /* if */
            CloseHandle(fileHandle);
          } /* if */
        } /* if */
      } /* if */
      os_stri_free(os_path);
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
  } /* cmdSetMTimeOfSymlink */



/**
 *  Set the owner of a file.
 *  The function follows symbolic links.
 *  @exception MEMORY_ERROR Not enough memory to convert 'filePath'
 *             to the system path type.
 *  @exception RANGE_ERROR 'filePath' does not use the standard path
 *             representation or it cannot be converted to the system
 *             path type.
 *  @exception FILE_ERROR The file described with 'filePath' does not
 *             exist, or a system function returns an error.
 */
void cmdSetOwner (const const_striType filePath, const const_striType owner)

  {
    os_striType os_path;
    int path_info = PATH_IS_NORMAL;
    errInfoType err_info = OKAY_NO_ERROR;
    HANDLE fileHandle;
    DWORD returnCode;
    PSID pSidOwner;

  /* cmdSetOwner */
    logFunction(printf("cmdSetOwner(\"%s\", ", striAsUnquotedCStri(filePath));
                printf("\"%s\")\n", striAsUnquotedCStri(owner)));
    os_path = cp_to_os_path(filePath, &path_info, &err_info);
    if (unlikely(os_path == NULL)) {
      logError(printf("cmdSetOwner: cp_to_os_path(\"%s\", *, *) failed:\n"
                      "path_info=%d, err_info=%d\n",
                      striAsUnquotedCStri(filePath), path_info, err_info););
    } else {
      fileHandle = CreateFileW(os_path, WRITE_OWNER, 0,
                               NULL, OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS, NULL);
      if (unlikely(fileHandle == INVALID_HANDLE_VALUE)) {
        logError(printf("cmdSetOwner(\"%s\"): "
                        "CreateFileW(\"" FMT_S_OS "\", *) failed:\n"
                        "lastError=" FMT_U32 "\n",
                        striAsUnquotedCStri(filePath),
                        os_path, (uint32Type) GetLastError()););
        err_info = FILE_ERROR;
      } else {
        pSidOwner = getSidFromName(owner, &err_info);
        if (likely(pSidOwner != NULL)) {
          returnCode = SetSecurityInfo(fileHandle, SE_FILE_OBJECT,
                                       OWNER_SECURITY_INFORMATION,
                                       pSidOwner, NULL, NULL, NULL);
          if (unlikely(returnCode != ERROR_SUCCESS)) {
            logError(printf("cmdSetOwner(\"%s\", ",
                            striAsUnquotedCStri(filePath));
                     printf("\"%s\"): "
                            "SetSecurityInfo(" FMT_U_MEM ", ...) failed:\n"
                            "returnCode=" FMT_U32 "\n",
                            striAsUnquotedCStri(owner),
                            (memSizeType) fileHandle,
                            (uint32Type) returnCode););
            err_info = FILE_ERROR;
          } /* if */
          free(pSidOwner);
        } /* if */
        CloseHandle(fileHandle);
      } /* if */
      os_stri_free(os_path);
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
  } /* cmdSetOwner */



/**
 *  Set the owner of a symbolic link.
 *  The function only works for symbolic links and does not follow the
 *  symbolic link.
 *  @exception MEMORY_ERROR Not enough memory to convert 'filePath'
 *             to the system path type.
 *  @exception RANGE_ERROR 'filePath' does not use the standard path
 *             representation or it cannot be converted to the system
 *             path type.
 *  @exception FILE_ERROR The file described with 'filePath' does not
 *             exist, or it is not a symbolic link, or a system function
 *             returns an error.
 */
void cmdSetOwnerOfSymlink (const const_striType filePath, const const_striType owner)

  {
    os_striType os_path;
    int path_info = PATH_IS_NORMAL;
    errInfoType err_info = OKAY_NO_ERROR;
    DWORD fileAttributes;
    DWORD returnCode;
    PSID pSidOwner;

  /* cmdSetOwnerOfSymlink */
    logFunction(printf("cmdSetOwnerOfSymlink(\"%s\", ", striAsUnquotedCStri(filePath));
                printf("\"%s\")\n", striAsUnquotedCStri(owner)));
    os_path = cp_to_os_path(filePath, &path_info, &err_info);
    if (unlikely(os_path == NULL)) {
#if MAP_ABSOLUTE_PATH_TO_DRIVE_LETTERS
      if (path_info == PATH_IS_EMULATED_ROOT) {
        /* The emulated root is not a symbolic link. */
        logError(printf("cmdSetOwnerOfSymlink(\"%s\"): "
                        "The emulated root is not a symbolic link.\n",
                        striAsUnquotedCStri(filePath)););
        err_info = FILE_ERROR;
      } else
#endif
      {
        logError(printf("cmdSetOwnerOfSymlink: cp_to_os_path(\"%s\", *, *) failed:\n"
                        "path_info=%d, err_info=%d\n",
                        striAsUnquotedCStri(filePath), path_info, err_info););
      }
    } else {
      if (unlikely((fileAttributes = GetFileAttributesW(os_path)) ==
                   INVALID_FILE_ATTRIBUTES)) {
        logError(printf("cmdSetOwnerOfSymlink(\"%s\", ...): "
                        "GetFileAttributesW(\"" FMT_S_OS "\") failed:\n"
                        "lastError=" FMT_U32 "\n",
                        striAsUnquotedCStri(filePath), os_path,
                        (uint32Type) GetLastError()););
        err_info = FILE_ERROR;
      } else if ((fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
        logError(printf("cmdSetOwnerOfSymlink(\"%s\", ...): "
                        "The file \"" FMT_S_OS "\" is not a symbolic link.\n",
                        striAsUnquotedCStri(filePath), os_path););
        err_info = FILE_ERROR;
      } else {
        pSidOwner = getSidFromName(owner, &err_info);
        if (likely(pSidOwner != NULL)) {
          returnCode = SetNamedSecurityInfoW(os_path, SE_FILE_OBJECT,
                                             OWNER_SECURITY_INFORMATION,
                                             pSidOwner, NULL, NULL, NULL);
          if (unlikely(returnCode != ERROR_SUCCESS)) {
            logError(printf("cmdSetOwnerOfSymlink(\"%s\", ",
                            striAsUnquotedCStri(filePath));
                     printf("\"%s\"): "
                            "SetNamedSecurityInfoW(\"" FMT_S_OS "\", ...) failed:\n"
                            "returnCode=" FMT_U32 "\n",
                            striAsUnquotedCStri(owner), os_path,
                            (uint32Type) returnCode););
            err_info = FILE_ERROR;
          } /* if */
          free(pSidOwner);
        } /* if */
      } /* if */
      os_stri_free(os_path);
    } /* if */
    if (unlikely(err_info != OKAY_NO_ERROR)) {
      raise_error(err_info);
    } /* if */
  } /* cmdSetOwnerOfSymlink */



striType cmdUser (void)

  {
    HANDLE hToken = NULL;
    TOKEN_USER *ptu;
    DWORD dwSize = 0;
    errInfoType err_info = OKAY_NO_ERROR;
    striType user;

  /* cmdUser */
    if (unlikely(OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken) == 0 &&
                 OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) == 0)) {
      logError(printf("cmdUser: OpenThreadToken() and OpenProcessToken() failed."););
      raise_error(FILE_ERROR);
      user = NULL;
    } else {
      if (unlikely(GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize) != 0 ||
                   ERROR_INSUFFICIENT_BUFFER != GetLastError())) {
        logError(printf("cmdUser: GetTokenInformation() "
                        "should fail with ERROR_INSUFFICIENT_BUFFER.\n"
                        "lastError=" FMT_U32 "\n",
                        (uint32Type) GetLastError()););
        raise_error(FILE_ERROR);
        user = NULL;
      } else {
        ptu = (TOKEN_USER *) LocalAlloc(LPTR, dwSize);
        if (unlikely(ptu == NULL)) {
          raise_error(MEMORY_ERROR);
          user = NULL;
        } else {
          if (GetTokenInformation(hToken, TokenUser, ptu, dwSize, &dwSize) == 0) {
            logError(printf("cmdUser: GetNamedSecurityInfoW() failed:\n"
                            "lastError=" FMT_U32 "\n",
                            (uint32Type) GetLastError()););
            LocalFree((HLOCAL) ptu);
            raise_error(FILE_ERROR);
            user = NULL;
          } else {
            user = getNameFromSid(ptu->User.Sid, &err_info);
            LocalFree((HLOCAL) ptu);
            if (unlikely(user == NULL)) {
              raise_error(err_info);
            } /* if */
          } /* if */
        } /* if */
      } /* if */
    } /* if */
    return user;
  } /* cmdUser */
