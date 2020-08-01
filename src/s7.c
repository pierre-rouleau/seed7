/********************************************************************/
/*                                                                  */
/*  s7   Seed7 interpreter                                          */
/*  Copyright (C) 1990 - 2015  Thomas Mertes                        */
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
/*  Module: Main                                                    */
/*  File: seed7/src/s7.c                                            */
/*  Changes: 1990 - 1994, 2010, 2011  Thomas Mertes                 */
/*  Content: Main program of the Seed7 interpreter.                 */
/*                                                                  */
/********************************************************************/

#define LOG_FUNCTIONS 0
#define VERBOSE_EXCEPTIONS 0

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "common.h"
#include "sigutl.h"
#include "data.h"
#include "data_rtl.h"
#include "infile.h"
#include "heaputl.h"
#include "striutl.h"
#include "syvarutl.h"
#include "identutl.h"
#include "entutl.h"
#include "findid.h"
#include "symbol.h"
#include "analyze.h"
#include "prg_comp.h"
#include "traceutl.h"
#include "exec.h"
#include "option.h"
#include "runerr.h"
#include "level.h"
#include "int_rtl.h"
#include "flt_rtl.h"
#include "arr_rtl.h"
#include "cmd_rtl.h"
#include "con_drv.h"
#include "fil_drv.h"

#ifdef USE_WINMAIN
typedef struct {
    int dummy;
  } HINSTANCE__;
typedef HINSTANCE__ *HINSTANCE;
#endif

striType programPath;

#ifdef CHECK_STACK
char *stack_base;
memSizeType max_stack_size = 0;
#endif

#define VERSION_INFO "SEED7 INTERPRETER Version 5.0.%d  Copyright (c) 1990-2015 Thomas Mertes\n"



void raise_error2 (int exception_num, const_cstriType filename, int line)

  { /* raise_error2 */
    /* printf("raise_error2(%d, %s, %d)\n", exception_num, filename, line); */
    (void) raise_exception(prog.sys_var[exception_num]);
  } /* raise_error2 */



static void writeHelp (void)

  { /* writeHelp */
    printf("usage: s7 [options] sourcefile [parameters]\n\n");
    printf("Options:\n");
    printf("  -?   Write Seed7 interpreter usage.\n");
    printf("  -a   Analyze only and suppress the execution phase.\n");
    printf("  -dx  Set compile time trace level to x. Where x is a string consisting of:\n");
    printf("         a Trace primitive actions\n");
    printf("         c Do action check\n");
    printf("         d Trace dynamic calls\n");
    printf("         e Trace exceptions and handlers\n");
    printf("         h Trace heap size (in combination with 'a')\n");
    printf("         s Trace signals\n");
    printf("  -d   Equivalent to -da\n");
    printf("  -i   Show the identifier table after the analyzing phase.\n");
    printf("  -l   Add a directory to the include library search path (e.g.: -l ../lib).\n");
    printf("  -p   Specify a protocol file, for trace output (e.g.: -p prot.txt).\n");
    printf("  -q   Compile quiet. Line and file information and compilation\n");
    printf("       statistics are suppressed.\n");
    printf("  -tx  Set runtime trace level to x. Where x is a string consisting of:\n");
    printf("         a Trace primitive actions\n");
    printf("         c Do action check\n");
    printf("         d Trace dynamic calls\n");
    printf("         e Trace exceptions and handlers\n");
    printf("         h Trace heap size (in combination with 'a')\n");
    printf("         s Trace signals\n");
    printf("  -t   Equivalent to -ta\n");
    printf("  -vn  Set verbosity level of analyse phase to n. Where n is one of:\n");
    printf("         0 Compile quiet (equivalent to -q)\n");
    printf("         1 Write just the header with version information (default)\n");
    printf("         2 Write a list of include libraries\n");
    printf("         3 Write line numbers, while analyzing\n");
    printf("  -v   Equivalent to -v2\n");
    printf("  -x   Execute even when the program contains errors.\n\n");
  } /* writeHelp */



#if ANY_LOG_ACTIVE
static void printArray (const const_rtlArrayType array)

  {
    memSizeType position;

  /* printArray */
    if (array == NULL) {
      printf("NULL");
    } else if (arraySize(array) != 0) {
      if (array->arr[0].value.striValue == NULL) {
        printf("NULL");
      } else {
        printf("\"%s\"", striAsUnquotedCStri(array->arr[0].value.striValue));
      } /* if */
      for (position = 1; position < arraySize(array); position++) {
        if (array->arr[position].value.striValue == NULL) {
          printf(", NULL");
        } else {
          printf(", \"%s\"", striAsUnquotedCStri(array->arr[position].value.striValue));
        } /* if */
      } /* for */
    } /* if */
    printf("\n");
  } /* printArray */



static void printOptions (const optionType option)

  { /* printOptions */
    printf("source_file_argument: \"%s\"\n",
           striAsUnquotedCStri(option->source_file_argument));
    printf("prot_file_name:       \"%s\"\n", striAsUnquotedCStri(option->prot_file_name));
    printf("write_help:           %s\n", option->write_help ? "TRUE" : "FALSE");
    printf("analyze_only:         %s\n", option->analyze_only ? "TRUE" : "FALSE");
    printf("execute_always:       %s\n", option->execute_always ? "TRUE" : "FALSE");
    printf("parser_options:       " FMT_U "\n", option->parser_options);
    printf("handle_signals:       " FMT_U "\n", option->handle_signals);
    printf("seed7_libraries:      ");
    printArray(option->seed7_libraries);
    printf("argv:                 ");
    printArray(option->argv);
    printf("argv_start:           " FMT_U_MEM "\n", option->argv_start);
  } /* printOptions */
#endif



static void processOptions (rtlArrayType arg_v, const optionType option)

  {
    int position;
    striType opt;
    striType trace_level;
    int verbosity_level = 1;
    rtlArrayType seed7_libraries;
    rtlObjectType path_obj;
    boolType error = FALSE;

  /* processOptions */
    logFunction(printf("processOptions\n"););
    option->source_file_argument = NULL;
    option->analyze_only = FALSE;
    if (ALLOC_RTL_ARRAY(seed7_libraries, 0)) {
      seed7_libraries->min_position = 1;
      seed7_libraries->max_position = 0;
    } /* if */
    for (position = 0; position < arg_v->max_position; position++) {
      if (option->source_file_argument == NULL) {
        opt = arg_v->arr[position].value.striValue;
        /* printf("opt=\"%s\"\n", striAsUnquotedCStri(opt)); */
        if (opt->size == 2 && opt->mem[0] == '-') {
          switch (opt->mem[1]) {
            case 'a':
              option->analyze_only = TRUE;
              break;
            case 'd':
              if (ALLOC_STRI_SIZE_OK(trace_level, 1)) {
                trace_level->mem[0] = 'a';
                trace_level->size = 1;
                mapTraceFlags(trace_level, &option->parser_options);
                FREE_STRI(trace_level, 1);
              } /* if */
              break;
            case 'h':
            case '?':
              option->write_help = TRUE;
              break;
            case 'i':
              option->parser_options |= SHOW_IDENT_TABLE;
              break;
            case 'p':
              if (position < arg_v->max_position - 1) {
                arg_v->arr[position].value.striValue = NULL;
                FREE_STRI(opt, opt->size);
                position++;
                opt = arg_v->arr[position].value.striValue;
                option->prot_file_name = stri_to_standard_path(opt);
                arg_v->arr[position].value.striValue = NULL;
                opt = NULL;
              } /* if */
              break;
            case 'q':
              verbosity_level = 0;
              break;
            case 's':
              option->handle_signals = FALSE;
              break;
            case 't':
              if (ALLOC_STRI_SIZE_OK(trace_level, 1)) {
                trace_level->mem[0] = 'a';
                trace_level->size = 1;
                mapTraceFlags(trace_level, &option->exec_options);
                FREE_STRI(trace_level, 1);
              } /* if */
              break;
            case 'v':
              verbosity_level = 2;
              break;
            case 'x':
              option->execute_always = TRUE;
              break;
            case 'l':
              if (position < arg_v->max_position - 1) {
                arg_v->arr[position].value.striValue = NULL;
                FREE_STRI(opt, opt->size);
                position++;
                opt = arg_v->arr[position].value.striValue;
                path_obj.value.striValue = stri_to_standard_path(opt);
                if (seed7_libraries != NULL) {
                  arrPush(&seed7_libraries, path_obj.value.genericValue);
                } /* if */
                arg_v->arr[position].value.striValue = NULL;
                opt = NULL;
              } /* if */
              break;
            default:
              if (!error) {
                printf(VERSION_INFO, LEVEL);
                error = TRUE;
              } /* if */
              printf("*** Ignore unsupported option: ");
              prot_stri_unquoted(opt);
              printf("\n");
              break;
          } /* switch */
        } else if (opt->size >= 3 && opt->mem[0] == '-') {
          switch (opt->mem[1]) {
            case 'd':
              if (ALLOC_STRI_SIZE_OK(trace_level, opt->size - 2)) {
                memcpy(trace_level->mem, &opt->mem[2],
                       (opt->size - 2) * sizeof(strElemType));
                trace_level->size = opt->size - 2;
                mapTraceFlags(trace_level, &option->parser_options);
                FREE_STRI(trace_level, 1);
              } /* if */
              break;
            case 't':
              if (ALLOC_STRI_SIZE_OK(trace_level, opt->size - 2)) {
                memcpy(trace_level->mem, &opt->mem[2],
                       (opt->size - 2) * sizeof(strElemType));
                trace_level->size = opt->size - 2;
                mapTraceFlags(trace_level, &option->exec_options);
                FREE_STRI(trace_level, 1);
              } /* if */
              break;
            case 'v':
              if (opt->mem[2] >= '0' && opt->mem[2] <= '3') {
                verbosity_level = (int) opt->mem[2] - '0';
              } else {
                verbosity_level = 2;
              } /* if */
              break;
            default:
              if (!error) {
                printf(VERSION_INFO, LEVEL);
                error = TRUE;
              } /* if */
              printf("*** Ignore unsupported option: ");
              prot_stri_unquoted(opt);
              printf("\n");
              break;
          } /* switch */
        } else {
          option->source_file_argument = stri_to_standard_path(opt);
          arg_v->arr[position].value.striValue = NULL;
          opt = NULL;
        } /* if */
        if (opt != NULL) {
          arg_v->arr[position].value.striValue = NULL;
          FREE_STRI(opt, opt->size);
        } /* if */
      } else {
        if (option->argv == NULL) {
          option->argv = arg_v;
          option->argv_start = (memSizeType) position;
          /* printf("argv_start = %d\n", position); */
        } /* if */
      } /* if */
    } /* for */
    option->seed7_libraries = seed7_libraries;
    if (verbosity_level >= 1) {
      if (verbosity_level >= 2) {
        option->parser_options |= WRITE_LIBRARY_NAMES;
        option->parser_options |= SHOW_STATISTICS;
        if (verbosity_level >= 3) {
          option->parser_options |= WRITE_LINE_NUMBERS;
        } /* if */
      } /* if */
      if (!error) {
        printf(VERSION_INFO, LEVEL);
      } /* if */
    } /* if */
    if (option->handle_signals) {
      option->parser_options |= HANDLE_SIGNALS;
      option->exec_options   |= HANDLE_SIGNALS;
    } /* if */
    logFunction(printf("processOptions -->\n");
                printOptions(option););
  } /* processOptions */



#ifdef USE_WMAIN
int wmain (int argc, wchar_t **argv)
#elif defined USE_WINMAIN
int WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, char *lpCmdLine, int nShowCmd)
#else
int main (int argc, char **argv)
#endif

  {
    rtlArrayType arg_v;
    progType currentProg;
    optionRecord option = {
        NULL,  /* source_file_name  */
        NULL,  /* prot_file_name    */
        FALSE, /* write_help        */
        FALSE, /* analyze_only      */
        FALSE, /* execute_always    */
        0,     /* parser_options    */
        0,     /* exec_options      */
        TRUE,  /* handle_signals    */
        NULL,  /* seed7_libraries   */
        NULL,  /* argv              */
        0,     /* argv_start        */
      };

  /* main */
    logFunction(printf("main\n"););
#ifdef CHECK_STACK
    stack_base = (char *) &arg_v;
#endif
    setupStack();
    setupFiles();
    set_protfile_name(NULL);
#ifdef USE_WINMAIN
    arg_v = getArgv(0, NULL, NULL, NULL, &programPath);
#else
    arg_v = getArgv(argc, argv, NULL, NULL, &programPath);
#endif
    if (arg_v == NULL) {
      printf(VERSION_INFO, LEVEL);
      printf("\n*** No more memory. Program terminated.\n");
    } else {
      processOptions(arg_v, &option);
      setup_signal_handlers((option.parser_options & HANDLE_SIGNALS) != 0,
                            (option.parser_options & TRACE_SIGNALS) != 0);
      if (fail_flag) {
        printf("\n*** Processing the options failed. Program terminated.\n");
      } else {
        if (arg_v->max_position < arg_v->min_position) {
          printf("This is free software; see the source for copying conditions.  There is NO\n");
          printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
          printf("Homepage: http://seed7.sourceforge.net\n\n");
          printf("usage: s7 [options] sourcefile [parameters]\n\n");
          printf("Use  s7 -?  to get more information about s7.\n\n");
        } else if (option.write_help) {
          writeHelp();
        } else {
          setupRand();
          setupFloat();
          /* printf("source_file_argument: \"");
             prot_stri(option.source_file_argument);
             printf("\"\n");
             printf("prot_file_name: \"%s\"\n", option.prot_file_name); */
          if (option.source_file_argument == NULL) {
            printf("*** Sourcefile missing\n");
          } else {
            currentProg = analyze(option.source_file_argument, option.parser_options,
                                  option.seed7_libraries, option.prot_file_name);
            if (!option.analyze_only && currentProg != NULL &&
                (currentProg->error_count == 0 || option.execute_always)) {
              /* PRIME_OBJECTS(); */
              /* printf("%d%d\n",
                 trace.actions,
                 trace.check_actions); */
              if (currentProg->main_object == NULL) {
                printf("*** Declaration for main missing\n");
              } else {
                interpret(currentProg, option.argv, option.argv_start,
                          option.exec_options, option.prot_file_name);
              } /* if */
              /* prgDestr(currentProg); */
              /* heap_statistic(); */
            } /* if */
          } /* if */
        } /* if */
        shut_drivers();
        if (fail_flag) {
          prot_nl();
          prot_cstri("*** Uncaught EXCEPTION ");
          printobject(fail_value);
          prot_cstri(" raised with");
          prot_nl();
          prot_list(fail_expression);
          prot_nl();
          prot_nl();
          prot_cstri("Stack:\n");
          write_call_stack(fail_stack);
        } /* if */
      } /* if */
    } /* if */
    /* getchar(); */
    /* heap_statistic(); */
#ifdef CHECK_STACK
    printf("max_stack_size: %lu (0x%lx)\n", max_stack_size, max_stack_size);
#endif
    logFunction(printf("main --> 0\n"););
    return 0;
  } /* main */
