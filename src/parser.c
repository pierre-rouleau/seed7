/********************************************************************/
/*                                                                  */
/*  s7   Seed7 interpreter                                          */
/*  Copyright (C) 1990 - 2013, 2015, 2024, 2025  Thomas Mertes      */
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
/*  Module: Analyzer - Parser                                       */
/*  File: seed7/src/parser.c                                        */
/*  Changes: 1990, 1991, 1992, 1993, 1994, 2015  Thomas Mertes      */
/*           2024, 2025  Thomas Mertes                              */
/*  Content: Main functions of the parser.                          */
/*                                                                  */
/********************************************************************/

#define LOG_FUNCTIONS 0
#define VERBOSE_EXCEPTIONS 0

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "common.h"
#include "data.h"
#include "heaputl.h"
#include "flistutl.h"
#include "syvarutl.h"
#include "striutl.h"
#include "identutl.h"
#include "listutl.h"
#include "typeutl.h"
#include "blockutl.h"
#include "actutl.h"
#include "traceutl.h"
#include "executl.h"
#include "objutl.h"
#include "runerr.h"
#include "fatal.h"
#include "infile.h"
#include "scanner.h"
#include "object.h"
#include "symbol.h"
#include "findid.h"
#include "error.h"
#include "atom.h"
#include "expr.h"
#include "stat.h"
#include "type.h"
#include "name.h"

#undef EXTERN
#define EXTERN
#include "parser.h"



static void init_dollar_type (objectType declared_object,
    typeType meta_type, errInfoType *err_info)

  {
    typeType generated_type;

  /* init_dollar_type */
    logFunction(printf("init_dollar_type\n"););
    if ((generated_type = new_type(prog, meta_type, NULL)) == NULL) {
      *err_info = MEMORY_ERROR;
    } else {
      SET_CATEGORY_OF_OBJ(declared_object, TYPEOBJECT);
      declared_object->value.typeValue = generated_type;
      /* printf("generated_type->name=%lu\n", generated_type->name); */
      if (generated_type->name == NULL &&
          HAS_ENTITY(declared_object) &&
          GET_ENTITY(declared_object)->ident != NULL) {
        generated_type->name = GET_ENTITY(declared_object)->ident;
        /* printf("typ_create: Define ");
        trace1(declared_object);
        printf("\n"); */
      } /* if */
    } /* if */
    logFunction(printf("init_dollar_type -->\n"););
  } /* init_dollar_type */



static inline void init_dollar (objectType declared_object,
    errInfoType *err_info)

  {
    objectType meta_type;
    objectType basic_type;
    typeType generated_type;

  /* init_dollar */
    logFunction(printf("init_dollar\n"););
    scan_symbol();
    if (current_ident == prog->id_for.newtype) {
      scan_symbol();
      init_dollar_type(declared_object, NULL, err_info);
      /* printf("newtype: declared_object: <%lx> ", declared_object);
      trace1(declared_object);
      printf("\n"); */
    } else if (current_ident == prog->id_for.subtype) {
      scan_symbol();
      meta_type = pars_infix_expression(SCOL_PRIORITY, TRUE);
      if (CATEGORY_OF_OBJ(meta_type) == TYPEOBJECT) {
        init_dollar_type(declared_object, take_type(meta_type), err_info);
      } else {
        err_object(TYPE_EXPECTED, meta_type);
        free_expression(meta_type);
      } /* if */
    } else if (current_ident == prog->id_for.func) {
      scan_symbol();
      basic_type = pars_infix_expression(SCOL_PRIORITY, TRUE);
      if (CATEGORY_OF_OBJ(basic_type) == TYPEOBJECT) {
        if ((generated_type = get_func_type(NULL,
            take_type(basic_type))) == NULL) {
          *err_info = MEMORY_ERROR;
        } else {
          SET_CATEGORY_OF_OBJ(declared_object, TYPEOBJECT);
          declared_object->value.typeValue = generated_type;
          if (generated_type->name == NULL &&
              HAS_ENTITY(declared_object) &&
              GET_ENTITY(declared_object)->ident != NULL) {
            generated_type->name = GET_ENTITY(declared_object)->ident;
            /* printf("typ_create: Define ");
            trace1(declared_object);
            printf("\n"); */
          } /* if */
        } /* if */
      } else {
        err_object(TYPE_EXPECTED, basic_type);
      } /* if */
    } else if (current_ident == prog->id_for.enumlit) {
      scan_symbol();
      SET_CATEGORY_OF_OBJ(declared_object, ENUMLITERALOBJECT);
      declared_object->value.objValue = NULL;
    } else if (current_ident == prog->id_for.action) {
      scan_symbol();
      if (symbol.sycategory == STRILITERAL) {
        SET_CATEGORY_OF_OBJ(declared_object, ACTOBJECT);
        declared_object->value.actValue = findAction(symbol.striValue);
        if (unlikely(declared_object->value.actValue == NULL)) {
          err_stri(UNDEFINED_ACTION, symbol.striValue);
          declared_object->value.actValue = getActIllegal();
        } /* if */
        scan_symbol();
      } else {
        err_warning(STRING_EXPECTED);
      } /* if */
    } else {
      err_warning(DOLLAR_VALUE_WRONG);
      if (current_ident != prog->id_for.semicolon) {
        scan_symbol();
      } /* if */
    } /* if */
    logFunction(printf("init_dollar --> " FMT_U_MEM ", ",
                       (memSizeType) declared_object);
                trace1(declared_object);
                printf("\n"););
  } /* init_dollar */



static void decl_value (objectType typeof_object, objectType declared_object,
    int is_dollar_type, errInfoType *err_info)

  {
    objectType init_expression;

  /* decl_value */
    logFunction(printf("decl_value\n"););
    if (current_ident == prog->id_for.is) {
      scan_symbol();
      if (current_ident == prog->id_for.dollar) {
        init_dollar(declared_object, err_info);
        /* printf("A declared_object: <%lx> ", declared_object);
        trace1(declared_object);
        printf("\n");
        printf("type before: <%lx> ", typeof_object);
        trace1(typeof_object);
        printf("\n"); */
        if (is_dollar_type) {
          typeof_object = declared_object;
        } /* if */
#ifdef OUT_OF_ORDER
        if (HAS_ENTITY(typeof_object) &&
            GET_ENTITY(typeof_object)->owner != NULL) {
          typeof_object = GET_ENTITY(typeof_object)->owner->obj;
          /* printf("type after: <%lx> ", typeof_object);
          trace1(typeof_object);
          printf("\n"); */
        } /* if */
#endif
        if (typeof_object != NULL) {
          if (CATEGORY_OF_OBJ(typeof_object) == TYPEOBJECT) {
            declared_object->type_of = take_type(typeof_object);
            if (is_dollar_type) {
              declared_object->type_of->is_type_type = TRUE;
            } /* if */
          } else {
            err_object(TYPE_EXPECTED, typeof_object);
          } /* if */
        } /* if */
#ifdef OUT_OF_ORDER
        if (is_dollar_type) {
          printf("const $ type is: ");
          trace1(declared_object);
          printf("\n");
        } /* if */
#endif
      } else {
        init_expression = pars_infix_expression(SCOL_PRIORITY, TRUE);
        if (init_expression != NULL) {
          if (typeof_object != NULL) {
            if (CATEGORY_OF_OBJ(typeof_object) == TYPEOBJECT) {
              declared_object->type_of = take_type(typeof_object);
              do_create(declared_object, init_expression, err_info);
              if (*err_info != OKAY_NO_ERROR) {
                if (*err_info != CREATE_ERROR) {
                  if (fail_file_number != 0) {
                    err_at_file_in_line(EXCEPTION_RAISED,
                                        prog->sys_var[*err_info],
                                        fail_file_number,
                                        fail_line_number);
                  } else {
                    err_expr_obj(EXCEPTION_RAISED, declared_object,
                                 prog->sys_var[*err_info]);
                  } /* if */
                } /* if */
                err_object(DECL_FAILED, declared_object);
                *err_info = OKAY_NO_ERROR;
              } /* if */
            } else {
              err_object(TYPE_EXPECTED, typeof_object);
            } /* if */
          } /* if */
          free_expression(init_expression);
        } /* if */
      } /* if */
    } else {
      err_ident(EXPECTED_SYMBOL, prog->id_for.is);
    } /* if */
    if (current_ident == prog->id_for.semicolon) {
      scan_symbol();
    } else {
      err_ident(EXPECTED_SYMBOL, prog->id_for.semicolon);
      do {
        scan_symbol();
      } while (current_ident != prog->id_for.semicolon &&
          symbol.sycategory != STOPSYMBOL);
      if (current_ident == prog->id_for.semicolon) {
        scan_symbol();
      } /* if */
    } /* if */
    logFunction(printf("decl_value -->\n"););
  } /* decl_value */



static objectType decl_name (nodeType node_level, errInfoType *err_info)

  {
    objectType object_name;
    objectType defined_object;

  /* decl_name */
    logFunction(printf("decl_name(" FMT_U_MEM ", %d)\n",
                       (memSizeType) node_level, *err_info););
    if (current_ident == prog->id_for.dollar) {
      scan_symbol();
      object_name = pars_infix_expression(COM_PRIORITY, FALSE);
      if (object_name != NULL) {
        defined_object = dollar_entername(node_level, object_name, err_info);
        free_expression(object_name);
      } else {
        defined_object = NULL;
      } /* if */
    } else {
      object_name = pars_infix_expression(COM_PRIORITY, FALSE);
      if (object_name != NULL) {
        defined_object = entername(node_level, object_name, err_info);
        free_expression(object_name);
      } else {
        defined_object = NULL;
      } /* if */
    } /* if */
    logFunction(printf("decl_name(" FMT_U_MEM ", %d) --> ",
                       (memSizeType) node_level, *err_info);
                trace1(defined_object);
                printf("\n"););
    return defined_object;
  } /* decl_name */



void decl_const (nodeType node_level, errInfoType *err_info)

  {
    objectType typeof_object;
    int is_dollar_type;
    objectType declared_object;

  /* decl_const */
    logFunction(printf("decl_const(" FMT_U_MEM ", %d)\n",
                       (memSizeType) node_level, *err_info););
    grow_stack(err_info);
    if (*err_info == OKAY_NO_ERROR) {
      typeof_object = decl_type(&is_dollar_type, err_info);
      declared_object = decl_name(node_level, err_info);
      if (declared_object != NULL && *err_info == OKAY_NO_ERROR) {
        if (HAS_PROPERTY(declared_object) &&
            PROPERTY_FILE_NUM(declared_object) == 0 &&
            PROPERTY_LINE_NUM(declared_object) == 0) {
          declared_object->descriptor.property->file_number = in_file.file_number;
          declared_object->descriptor.property->line = in_file.line;
	} /* if */
        decl_value(typeof_object, declared_object, is_dollar_type, err_info);
      } /* if */
      shrink_stack();
    } /* if */
    logFunction(printf("decl_const(" FMT_U_MEM ", %d) -->\n",
                       (memSizeType) node_level, *err_info););
  } /* decl_const */
