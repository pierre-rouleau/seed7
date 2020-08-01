/********************************************************************/
/*                                                                  */
/*  s7   Seed7 interpreter                                          */
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
/*  License along with this program; if not, write to the           */
/*  Free Software Foundation, Inc., 51 Franklin Street,             */
/*  Fifth Floor, Boston, MA  02110-1301, USA.                       */
/*                                                                  */
/*  Module: Runtime                                                 */
/*  File: seed7/src/runerr.c                                        */
/*  Changes: 1990, 1991, 1992, 1993, 1994  Thomas Mertes            */
/*  Content: Runtime error and exception handling procedures.       */
/*                                                                  */
/********************************************************************/

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "signal.h"

#include "common.h"
#include "data.h"
#include "heaputl.h"
#include "flistutl.h"
#include "syvarutl.h"
#include "datautl.h"
#include "listutl.h"
#include "sigutl.h"
#include "actutl.h"
#include "traceutl.h"
#include "infile.h"
#include "exec.h"
#include "rtl_err.h"

#undef EXTERN
#define EXTERN
#include "runerr.h"



void continue_question (void)

  {
    int ch;
    int position;
    char buffer[10];
    long unsigned int exception_num;

  /* continue_question */
    printf("\n*** The following commands are possible:\n"
           "  RETURN  Continue\n"
           "  *       Terminate\n"
           "  /       Trigger SIGFPE\n"
           "  !n      Raise exception with number (e.g.: !1 raises MEMORY_ERROR)\n");
    ch = fgetc(stdin);
    if (ch == (int) '*') {
      shut_drivers();
      exit(1);
    } else if (ch == (int) '/') {
      /* signal(SIGFPE, SIG_DFL); */
      position = 0;
#ifdef DO_SIGFPE_WITH_DIV_BY_ZERO
      printf("%d", 1 / position); /* trigger SIGFPE on purpose */
#else
      raise(SIGFPE);
#endif
      printf("\n*** Raising SIGFPE failed.\n");
    } /* if */
    position = 0;
    while (ch >= (int) ' ' && ch <= (int) '~' && position < 9) {
      buffer[position] = (char) ch;
      position++;
      ch = fgetc(stdin);
    } /* while */
    buffer[position] = '\0';
    if (position > 0 && buffer[0] == '!') {
      if (buffer[1] >= '0' && buffer[1] <= '9') {
        exception_num = strtoul(&buffer[1], NULL, 10);
        if (exception_num > OKAY_NO_ERROR && exception_num <= ACTION_ERROR) {
          raise_exception(prog.sys_var[exception_num]);
        } /* if */
      } else {
        mapTraceFlags2(&buffer[1], &prog.option_flags);
        set_trace(prog.option_flags);
      } /* if */
    } /* if */
    while (ch != EOF && ch != '\n') {
      ch = fgetc(stdin);
    } /* while */
  } /* continue_question */



static void write_call_stack_element (const_listType stack_elem)

  {
    const_listType position_stack_elem;
    objectType func_object;

  /* write_call_stack_element */
    if (stack_elem->obj != NULL) {
      if (stack_elem->next != NULL) {
        if (CATEGORY_OF_OBJ(stack_elem->obj) == CALLOBJECT ||
            CATEGORY_OF_OBJ(stack_elem->obj) == MATCHOBJECT) {
          func_object = stack_elem->obj->value.listValue->obj;
        } else {
          func_object = stack_elem->obj;
        } /* if */
        if (HAS_ENTITY(func_object)) {
          printf("in ");
          if (GET_ENTITY(func_object)->ident != NULL) {
            printf("%s ", id_string(GET_ENTITY(func_object)->ident));
          } else if (func_object->descriptor.property->params != NULL) {
            prot_params(func_object->descriptor.property->params);
            printf(" ");
          } else if (GET_ENTITY(func_object)->fparam_list != NULL) {
            prot_name(GET_ENTITY(func_object)->fparam_list);
            printf(" ");
          } /* if */
        } /* if */
        position_stack_elem = stack_elem->next;
        if (HAS_POSINFO(position_stack_elem->obj)) {
          /*
          printf("\n");
          trace1(position_stack_elem->obj);
          printf("\n");
          trace1(func_object);
          printf("\n");
          */
          printf("at %s(%u)\n",
              get_file_name_ustri(GET_FILE_NUM(position_stack_elem->obj)),
              GET_LINE_NUM(position_stack_elem->obj));
        } else if (HAS_PROPERTY(position_stack_elem->obj) &&
            position_stack_elem->obj->descriptor.property->line != 0) {
          printf("at %s(%u)\n",
              get_file_name_ustri(position_stack_elem->obj->descriptor.property->file_number),
              position_stack_elem->obj->descriptor.property->line);
        } else {
          printf("no POSITION INFORMATION ");
          /* trace1(position_stack_elem->obj); */
          printf("\n");
        } /* if */
      } /* if */
    } else {
      printf("NULL\n");
    } /* if */
  } /* write_call_stack_element */



void write_call_stack (const_listType stack_elem)

  { /* write_call_stack */
    if (stack_elem != NULL) {
      write_call_stack(stack_elem->next);
      write_call_stack_element(stack_elem);
    } /* if */
  } /* write_call_stack */



static void write_curr_position (listType list)

  { /* write_curr_position */
    if (list == curr_argument_list) {
      if (curr_action_object != NULL) {
        if (HAS_POSINFO(curr_action_object)) {
          printf(" at %s(%u)",
              get_file_name_ustri(GET_FILE_NUM(curr_action_object)),
              GET_LINE_NUM(curr_action_object));
        } else if (HAS_PROPERTY(curr_action_object) &&
            curr_action_object->descriptor.property->line != 0) {
          printf(" at %s(%u)",
              get_file_name_ustri(curr_action_object->descriptor.property->file_number),
              curr_action_object->descriptor.property->line);
        } /* if */
      } /* if */
      printf("\n");
      prot_list(list);
      if (curr_exec_object != NULL) {
        if (HAS_POSINFO(curr_exec_object)) {
          printf(" at %s(%u)",
              get_file_name_ustri(GET_FILE_NUM(curr_exec_object)),
              GET_LINE_NUM(curr_exec_object));
        } else if (HAS_PROPERTY(curr_exec_object) &&
            curr_exec_object->descriptor.property->line != 0) {
          printf(" at %s(%u)",
              get_file_name_ustri(curr_exec_object->descriptor.property->file_number),
              curr_exec_object->descriptor.property->line);
        } /* if */
      } /* if */
      printf("\n");
      if (curr_action_object->value.actValue != NULL) {
        printf("*** ACTION \"%s\"\n",
            get_primact(curr_action_object->value.actValue)->name);
      } /* if */
    } else {
      printf(" with\n");
      prot_list(list);
    } /* if */
  } /* write_curr_position */



objectType raise_with_arguments (objectType exception, listType list)

  {
    errInfoType err_info = OKAY_NO_ERROR;

  /* raise_with_arguments */
#ifdef WITH_PROTOCOL
    if (list == curr_argument_list) {
      if (curr_exec_object != NULL && curr_exec_object->value.listValue != NULL) {
        curr_action_object = curr_exec_object->value.listValue->obj;
        incl_list(&fail_stack, curr_action_object, &err_info);
      } /* if */
    } /* if */
    if (trace.exceptions) {
      prot_nl();
      prot_cstri("*** EXCEPTION ");
      printobject(exception);
      printf(" raised");
      write_curr_position(list);
      continue_question();
    } /* if */
#endif
#ifndef USE_CHUNK_ALLOCS
    if (exception == SYS_MEM_EXCEPTION) {
      reuse_free_lists();
    } /* if */
#endif
    if (exception == NULL) {
      if (ALLOC_OBJECT(exception)) {
        exception->type_of = NULL;
        exception->descriptor.property = NULL;
        INIT_CATEGORY_OF_TEMP(exception, SYMBOLOBJECT);
        exception->value.intValue = 0;
      } /* if */
    } /* if */
    incl_list(&fail_stack, curr_exec_object, &err_info);
    if (!fail_flag || fail_value == NULL) {
      fail_value = exception;
      fail_expression = copy_list(list, &err_info);
    } /* if */
    set_fail_flag(TRUE);
    return NULL;
  } /* raise_with_arguments */



objectType raise_exception (objectType exception)

  { /* raise_exception */
    return raise_with_arguments(exception, curr_argument_list);
  } /* raise_exception */



void interprRaiseError (int exception_num, const_cstriType filename, int line)

  { /* interprRaiseError */
    (void) raise_exception(prog.sys_var[exception_num]);
  } /* interprRaiseError */



void show_signal (void)

  { /* show_signal */
    interrupt_flag = FALSE;
    printf("\n*** Program stopped with signal %s\n", signal_name(signal_number));
    continue_question();
  } /* show_signal */



void run_error (objectCategory required, objectType argument)

  { /* run_error */
    if (curr_exec_object != NULL) {
      curr_action_object = curr_exec_object->value.listValue->obj;
    } /* if */
    printf("\n*** ACTION $");
    if (curr_action_object->value.actValue != NULL) {
      printf("%s", get_primact(curr_action_object->value.actValue)->name);
    } else {
      printf("NULL");
    } /* if */
    if (HAS_POSINFO(curr_action_object)) {
      printf(" AT %s(%u)",
          get_file_name_ustri(GET_FILE_NUM(curr_action_object)),
          GET_LINE_NUM(curr_action_object));
    } else if (HAS_PROPERTY(curr_action_object) &&
        curr_action_object->descriptor.property->line != 0) {
      printf(" AT %s(%u)",
          get_file_name_ustri(curr_action_object->descriptor.property->file_number),
          curr_action_object->descriptor.property->line);
    } /* if */
    printf(" REQUIRES ");
    printcategory(required);
    printf(" NOT ");
    printcategory(CATEGORY_OF_OBJ(argument));
    printf("\n");
    trace1(argument);
    printf("\n");
    prot_list(curr_argument_list);
    continue_question();
    raise_error(RANGE_ERROR);
  } /* run_error */



void empty_value (objectType argument)

  { /* empty_value */
    if (curr_exec_object != NULL) {
      curr_action_object = curr_exec_object->value.listValue->obj;
    } /* if */
    printf("\n*** ACTION $");
    if (curr_action_object->value.actValue != NULL) {
      printf("%s", get_primact(curr_action_object->value.actValue)->name);
    } else {
      printf("NULL");
    } /* if */
    printf(" WITH EMPTY VALUE\n");
    trace1(argument);
    printf("\nobject_ptr=" FMT_X_MEM "\n", (memSizeType) argument);
    prot_list(curr_argument_list);
    continue_question();
    raise_error(RANGE_ERROR);
  } /* empty_value */



void var_required (objectType argument)

  { /* var_required */
    if (curr_exec_object != NULL) {
      curr_action_object = curr_exec_object->value.listValue->obj;
    } /* if */
    printf("\n*** ACTION $");
    if (curr_action_object->value.actValue != NULL) {
      printf("%s", get_primact(curr_action_object->value.actValue)->name);
    } else {
      printf("NULL");
    } /* if */
    printf(" REQUIRES VARIABLE ");
    printcategory(CATEGORY_OF_OBJ(argument));
    printf(" NOT CONSTANT\n");
    trace1(argument);
    printf("\nobject_ptr=" FMT_X_MEM "\n", (memSizeType) argument);
    prot_list(curr_argument_list);
    continue_question();
    raise_error(RANGE_ERROR);
  } /* var_required */
