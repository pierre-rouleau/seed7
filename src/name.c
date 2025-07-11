/********************************************************************/
/*                                                                  */
/*  s7   Seed7 interpreter                                          */
/*  Copyright (C) 1990 - 2013, 2015, 2020, 2021  Thomas Mertes      */
/*                2024, 2025  Thomas Mertes                         */
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
/*  Module: Analyzer                                                */
/*  File: seed7/src/name.c                                          */
/*  Changes: 1994, 2011, 2013, 2015, 2020, 2021  Thomas Mertes      */
/*           2024, 2025  Thomas Mertes                              */
/*  Content: Enter an object in a specified declaration level.      */
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
#include "identutl.h"
#include "objutl.h"
#include "entutl.h"
#include "typeutl.h"
#include "listutl.h"
#include "traceutl.h"
#include "executl.h"
#include "exec.h"
#include "actutl.h"
#include "dcllib.h"
#include "findid.h"
#include "match.h"
#include "error.h"

#undef EXTERN
#define EXTERN
#include "name.h"


static int data_depth = 0;
static int depth = 0;



static void push_owner (ownerType *owner, objectType obj_to_push,
    stackType decl_level, errInfoType *err_info)

  {
    ownerType created_owner;

  /* push_owner */
    logFunction(printf("push_owner " FMT_U_MEM ", ",
                       (memSizeType) obj_to_push);
                trace1(obj_to_push);
                printf("\n"););
    if (ALLOC_RECORD(created_owner, ownerRecord, count.owner)) {
      created_owner->obj = obj_to_push;
      created_owner->decl_level = decl_level;
      created_owner->next = *owner;
      *owner = created_owner;
    } else {
      *err_info = MEMORY_ERROR;
    } /* if */
    logFunction(printf("push_owner --> " FMT_U_MEM ", ",
                       (memSizeType) obj_to_push);
                trace1(obj_to_push);
                printf("\n"););
  } /* push_owner */



static void pop_owner (ownerType *owner)

  {
    ownerType old_owner;

  /* pop_owner */
    logFunction(printf("pop_owner\n"););
    old_owner = *owner;
    *owner = old_owner->next;
    FREE_RECORD(old_owner, ownerRecord, count.owner);
    logFunction(printf("pop_owner -->\n"););
  } /* pop_owner */



static objectType get_object (progType currentProg, entityType entity,
    listType params, fileNumType file_number, lineNumType line,
    errInfoType *err_info)

  {
    objectType defined_object;
    objectType forward_reference;
    propertyType defined_property;

  /* get_object */
    logFunction(printf("get_object(");
                prot_list(params);
                printf(", %d, %d, %d)\n", file_number, line, *err_info););
    if (entity->data.owner != NULL &&
        entity->data.owner->decl_level == currentProg->stack_current) {
      defined_object = entity->data.owner->obj;
      if (CATEGORY_OF_OBJ(defined_object) != FORWARDOBJECT) {
        err_at_file_in_line(REDECLARATION, defined_object, file_number, line);
        err_existing_obj(PREVIOUS_DECLARATION, defined_object);
        free_list(params);
        defined_object = NULL;
      } else {
        SET_CATEGORY_OF_OBJ(defined_object, DECLAREDOBJECT);
        /* The old parameter names could be checked against the new ones. */
        free_params(currentProg, defined_object->descriptor.property->params);
        defined_object->descriptor.property->params = params;
        defined_object->descriptor.property->file_number = file_number;
        defined_object->descriptor.property->line = line;
        /* defined_object->descriptor.property->syNumberInLine = symbol.syNumberInLine; */
        if (ALLOC_OBJECT(forward_reference)) {
          forward_reference->type_of = NULL;
          forward_reference->descriptor.property = NULL;
          INIT_CATEGORY_OF_OBJ(forward_reference, FWDREFOBJECT);
          forward_reference->value.objValue = defined_object;
          replace_list_elem(currentProg->stack_current->local_object_list,
                            defined_object, forward_reference);
          currentProg->stack_current->object_list_insert_place = append_element_to_list(
              currentProg->stack_current->object_list_insert_place, defined_object, err_info);
          if (*err_info != OKAY_NO_ERROR) {
            replace_list_elem(currentProg->stack_current->local_object_list,
                              forward_reference, defined_object);
            FREE_OBJECT(forward_reference);
          } /* if */
        } else {
          *err_info = MEMORY_ERROR;
        } /* if */
      } /* if */
    } else {
      if (ALLOC_OBJECT(defined_object)) {
        if (ALLOC_PROPERTY(defined_property)) {
          defined_property->entity = entity;
          defined_property->params = params;
          defined_property->file_number = file_number;
          defined_property->line = line;
          /* defined_property->syNumberInLine = symbol.syNumberInLine; */
          defined_object->type_of = NULL;
          defined_object->descriptor.property = defined_property;
          INIT_CATEGORY_OF_OBJ(defined_object, DECLAREDOBJECT);
          defined_object->value.objValue = NULL;
          push_owner(&entity->data.owner, defined_object, currentProg->stack_current, err_info);
          if (*err_info == OKAY_NO_ERROR) {
            currentProg->stack_current->object_list_insert_place = append_element_to_list(
                currentProg->stack_current->object_list_insert_place, defined_object, err_info);
            if (*err_info != OKAY_NO_ERROR) {
              pop_owner(&entity->data.owner);
              FREE_PROPERTY(defined_property);
              FREE_OBJECT(defined_object);
              free_list(params);
              defined_object = NULL;
            } /* if */
          } else {
            FREE_PROPERTY(defined_property);
            FREE_OBJECT(defined_object);
            free_list(params);
            defined_object = NULL;
          } /* if */
        } else {
          FREE_OBJECT(defined_object);
          free_list(params);
          *err_info = MEMORY_ERROR;
          defined_object = NULL;
        } /* if */
      } else {
        free_list(params);
        *err_info = MEMORY_ERROR;
      } /* if */
    } /* if */
    logFunction(printf("get_object --> " FMT_U_MEM ", ",
                       (memSizeType) defined_object);
                trace1(defined_object);
                printf("\n"););
    return defined_object;
  } /* get_object */



void free_name (objectType object)

  { /* free_name */
    if (object != NULL) {
      if (HAS_PROPERTY(object) &&
          object->descriptor.property != prog->property.literal) {
        free_params(prog, object->descriptor.property->params);
        FREE_PROPERTY(object->descriptor.property);
      } /* if */
      FREE_OBJECT(object);
    } /* if */
  } /* free_name */



static listType create_parameter_list (listType name_list, errInfoType *err_info)

  {
    listType name_elem;
    objectType param_obj;
    listType parameter_list;
    listType *list_insert_place;

  /* create_parameter_list */
    logFunction(printf("create_parameter_list(");
                prot_list(name_list);
                printf(")\n"););
    name_elem = name_list;
    parameter_list = NULL;
    list_insert_place = &parameter_list;
    while (name_elem != NULL) {
      if (CATEGORY_OF_OBJ(name_elem->obj) == FORMPARAMOBJECT) {
        logMessage(printf("create paramobject " FMT_U_MEM " ",
                          (memSizeType) name_elem->obj);
                   trace1(name_elem->obj);
                   printf("\n"););
        param_obj = name_elem->obj->value.objValue;
        if (CATEGORY_OF_OBJ(param_obj) == VALUEPARAMOBJECT ||
            CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT) {
          logMessage(printf("create in or ref param " FMT_U_MEM " ",
                            (memSizeType) param_obj);
                     trace1(param_obj);
                     printf("\n"););
          list_insert_place = append_element_to_list(list_insert_place,
              param_obj, err_info);
        } else if (CATEGORY_OF_OBJ(param_obj) == TYPEOBJECT) {
          logMessage(printf("create attr param " FMT_U_MEM " ",
                            (memSizeType) param_obj);
                     trace1(param_obj);
                     printf("\n"););
          list_insert_place = append_element_to_list(list_insert_place,
              param_obj, err_info);
        } else {
          logMessage(printf("create symbol param " FMT_U_MEM " ",
                            (memSizeType) param_obj);
                     trace1(param_obj);
                     printf("\n"););
          list_insert_place = append_element_to_list(list_insert_place,
              param_obj, err_info);
        } /* if */
      } else {
        logMessage(printf("create symbol param " FMT_U_MEM " ",
                          (memSizeType) name_elem->obj);
                   trace1(name_elem->obj);
                   printf("\n"););
        list_insert_place = append_element_to_list(list_insert_place,
            name_elem->obj, err_info);
      } /* if */
      name_elem = name_elem->next;
    } /* while */
    logFunction(printf("create_parameter_list\n"););
    return parameter_list;
  } /* create_parameter_list */



static void free_form_param_list (listType name_list)

  {
    listType name_elem;
    listType list_end;

  /* free_form_param_list */
    logFunction(printf("free_form_param_list\n"););
    if (name_list != NULL) {
      name_elem = name_list;
      do {
        logMessage(printf("free_form_param_list: ");
                   trace1(name_elem->obj);
                   printf("\n"););
        if (CATEGORY_OF_OBJ(name_elem->obj) == FORMPARAMOBJECT) {
          FREE_OBJECT(name_elem->obj);
        } /* if */
        list_end = name_elem;
        name_elem = name_elem->next;
      } while (name_elem != NULL);
      free_list2(name_list, list_end);
    } /* if */
    logFunction(printf("free_form_param_list -->\n"););
  } /* free_form_param_list */



static void free_name_list (listType name_list)

  {
    listType name_elem;
    objectType param_obj;

  /* free_name_list */
    logFunction(printf("free_name_list\n"););
    name_elem = name_list;
    while (name_elem != NULL) {
      if (CATEGORY_OF_OBJ(name_elem->obj) == FORMPARAMOBJECT) {
        logMessage(printf("free_name_list: ");
                   trace1(name_elem->obj);
                   printf("\n"););
        param_obj = name_elem->obj->value.objValue;
        if (CATEGORY_OF_OBJ(param_obj) == VALUEPARAMOBJECT ||
            CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT) {
          logMessage(printf("free param_obj " FMT_U_MEM " %d %d: ",
                            (memSizeType) param_obj,
                            HAS_ENTITY(param_obj),
                            HAS_PROPERTY(param_obj));
                     trace1(param_obj);
                     printf("\n"););
          if (!HAS_ENTITY(param_obj) ||
              GET_ENTITY(param_obj)->data.owner == NULL ||
              GET_ENTITY(param_obj)->data.owner->obj != param_obj) {
            logMessage(printf("free " FMT_U_MEM " ", (memSizeType) param_obj);
                       trace1(param_obj);
                       printf("\n"););
            if (HAS_PROPERTY(param_obj) && param_obj->descriptor.property != prog->property.literal) {
              /* free_params(prog, param_obj->descriptor.property->params); */
              FREE_PROPERTY(param_obj->descriptor.property);
            } /* if */
            FREE_OBJECT(param_obj);
          } /* if */
        } /* if */
        FREE_OBJECT(name_elem->obj);
      } /* if */
      name_elem = name_elem->next;
    } /* while */
    free_list(name_list);
    logFunction(printf("free_name_list -->\n"););
  } /* free_name_list */



static objectType push_name (progType currentProg, nodeType declaration_base,
    listType name_list, fileNumType file_number, lineNumType line,
    errInfoType *err_info)

  {
    entityType entity;
    listType params;
    objectType defined_object;

  /* push_name */
    logFunction(printf("push_name(" FMT_U_MEM ", ",
                       (memSizeType) declaration_base);
                prot_list(name_list);
                printf(")\n"););
    params = create_parameter_list(name_list, err_info);
    if (*err_info != OKAY_NO_ERROR) {
      free_list(params);
      defined_object = NULL;
    } else {
      entity = get_entity(declaration_base, name_list);
      if (entity == NULL) {
        free_list(params);
        *err_info = MEMORY_ERROR;
        defined_object = NULL;
      } else {
        defined_object = get_object(currentProg, entity, params,
            file_number, line, err_info);
        if (*err_info != OKAY_NO_ERROR) {
          if (entity->fparam_list == name_list) {
            /* A new entity has been created by get_entity. */
            pop_entity(declaration_base, entity);
            FREE_RECORD(entity, entityRecord, count.entity);
          } /* if */
        } else if (defined_object == NULL) {
          /* An object has been declared twice. */
          push_stack();
          /* Remove the parameters from the stack: */
          pop_stack();
          free_name_list(name_list);
        } else if (entity->fparam_list != name_list) {
          /* An existing entity is used. */
          free_form_param_list(name_list);
        } /* if */
      } /* if */
    } /* if */
    /* prot_list(GET_ENTITY(defined_object)->params); */
    logFunction(printf("push_name --> " FMT_U_MEM "\n",
                       (memSizeType) defined_object););
    return defined_object;
  } /* push_name */



static void pop_object (progType currentProg, const_objectType obj_to_pop)

  {
    entityType entity;
    ownerType owner;

  /* pop_object */
    logFunction(printf("pop_object(" FMT_U_MEM " ",
                       (memSizeType) obj_to_pop);
                trace1(obj_to_pop);
                printf(")\n");
                fflush(stdout););
    if (HAS_ENTITY(obj_to_pop)) {
      entity = GET_ENTITY(obj_to_pop);
      owner = entity->data.owner;
      if (owner != NULL) {
        entity->data.owner = owner->next;
        FREE_RECORD(owner, ownerRecord, count.owner);
        if (entity->data.owner == NULL && entity->fparam_list != NULL) {
          pop_entity(currentProg->declaration_root, entity);
          entity->data.next = currentProg->entity.inactive_list;
          currentProg->entity.inactive_list = entity;
        } /* if */
      } /* if */
    } /* if */
    logFunction(printf("pop_object -->\n"););
  } /* pop_object */



static void disconnect_entity (const objectType anObject)

  {
    entityType entity;
    stackType decl_lev;
    listType *lstptr;
    listType lst;
    listType old_elem;

  /* disconnect_entity */
    logFunction(printf("disconnect_entity\n"););
    entity = GET_ENTITY(anObject);
    if (entity->data.owner != NULL && entity->data.owner->obj == anObject) {
      /* printf("disconnect_entity ");
         trace1(anObject);
         printf("\n"); */
      decl_lev = entity->data.owner->decl_level;
      lstptr = &decl_lev->local_object_list;
      lst = decl_lev->local_object_list;
      while (lst != NULL) {
        if (lst->obj == anObject) {
          if (decl_lev->object_list_insert_place == &lst->next) {
            decl_lev->object_list_insert_place = lstptr;
          } /* if */
          old_elem = lst;
          *lstptr = lst->next;
          lst = lst->next;
          FREE_L_ELEM(old_elem);
        } else {
          lstptr = &lst->next;
          lst = lst->next;
        } /* if */
      } /* while */
      pop_object(prog, anObject);
      FREE_PROPERTY(anObject->descriptor.property);
      anObject->descriptor.property = NULL;
    } /* if */
    logFunction(printf("disconnect_entity -->\n"););
  } /* disconnect_entity */



void disconnect_param_entities (const const_objectType objWithParams)

  {
    listType param_elem;
    objectType param_obj;

  /* disconnect_param_entities */
    logFunction(printf("disconnect_param_entities(");
                trace1(objWithParams);
                printf(")\n"););
    if (FALSE && HAS_PROPERTY(objWithParams)) {
      param_elem = objWithParams->descriptor.property->params;
      while (param_elem != NULL) {
        param_obj = param_elem->obj;
        if (CATEGORY_OF_OBJ(param_obj) == VALUEPARAMOBJECT ||
            CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT) {
          if (HAS_ENTITY(param_obj)) {
            disconnect_entity(param_obj);
          } /* if */
        } /* if */
        param_elem = param_elem->next;
      } /* while */
    } /* if */
    logFunction(printf("disconnect_param_entities -->\n"););
  } /* disconnect_param_entities */



void init_stack (progType currentProg, errInfoType *err_info)

  {
    stackType created_stack_element;

  /* init_stack */
    logFunction(printf("init_stack\n"););
    if (ALLOC_RECORD(created_stack_element, stackRecord, count.stack)) {
      created_stack_element->upward = NULL;
      created_stack_element->downward = NULL;
      created_stack_element->local_object_list = NULL;
      created_stack_element->object_list_insert_place =
          &created_stack_element->local_object_list;
      currentProg->stack_global = created_stack_element;
      currentProg->stack_data = created_stack_element;
      currentProg->stack_current = created_stack_element;
      data_depth = 1;
      depth = 1;
    } else {
      *err_info = MEMORY_ERROR;
    } /* if */
    logFunction(printf("init_stack -->\n"););
  } /* init_stack */



static void close_current_stack (progType currentProg)

  {
    listType list_end;
    listType reversed_list;
    listType list_element;

  /* close_current_stack */
    logFunction(printf("close_current_stack %d\n", data_depth););
    /* The list of objects is reversed to free the objects in */
    /* the opposite way of their definition.                  */
    list_end = currentProg->stack_current->local_object_list;
    if (list_end != NULL) {
      reversed_list = reverse_list(list_end);
      list_element = reversed_list;
      while (list_element != NULL) {
        if (CATEGORY_OF_OBJ(list_element->obj) != BLOCKOBJECT) {
          logMessage(printf(FMT_U_MEM " ",
                            (memSizeType) list_element->obj);
                     trace1(list_element->obj);
                     printf("\n"););
          dump_temp_value(list_element->obj);
          pop_object(currentProg, list_element->obj);
        } /* if */
        list_element = list_element->next;
      } /* while */
      list_element = reversed_list;
      while (list_element != NULL) {
        if (CATEGORY_OF_OBJ(list_element->obj) == BLOCKOBJECT) {
          logMessage(printf(FMT_U_MEM " ",
                            (memSizeType) list_element->obj);
                     trace1(list_element->obj);
                     printf("\n"););
          dump_temp_value(list_element->obj);
          SET_CATEGORY_OF_OBJ(list_element->obj, ACTOBJECT);
          list_element->obj->value.actValue = getActIllegal();
          pop_object(currentProg, list_element->obj);
        } /* if */
        list_element = list_element->next;
      } /* while */
      list_element = reversed_list;
      /* Freeing objects in an extra loop avoids accessing      */
      /* freed object data. In case of forward declared objects */
      /* the category of a freed object would be accessed.      */
      while (list_element != NULL) {
        if (HAS_PROPERTY(list_element->obj) &&
            list_element->obj->descriptor.property !=
            currentProg->property.literal) {
          /* In destr_interface() the STRUCTOBJECT value of an  */
          /* interface is freed when usage_count reaches zero.  */
          /* STRUCTOBJECT values with properties are treated    */
          /* special by destr_interface(). In this case the     */
          /* struct value of the STRUCTOBJECT is freed and set  */
          /* to NULL but the STRUCTOBJECT and its properties    */
          /* are left intact. In this case sct_destr() just     */
          /* sets the UNUSED flag of the STRUCTOBJECT. The      */
          /* object and the descriptor.property are freed here. */
          free_params(currentProg,
                      list_element->obj->descriptor.property->params);
          FREE_PROPERTY(list_element->obj->descriptor.property);
        } /* if */
        FREE_OBJECT(list_element->obj);
        list_element = list_element->next;
      } /* while */
      free_list2(reversed_list, list_end);
      currentProg->stack_current->local_object_list = NULL;
    } /* if */
    logFunction(printf("close_current_stack %d -->\n", data_depth););
  } /* close_current_stack */



void close_stack (progType currentProg)

  { /* close_stack */
    logFunction(printf("close_stack %d\n", data_depth););
    /*
    printf("stack_global:   " FMT_U_MEM "\n", (memSizeType) currentProg->stack_global);
    printf("stack_data:     " FMT_U_MEM "\n", (memSizeType) currentProg->stack_data);
    printf("stack_current:  " FMT_U_MEM "\n", (memSizeType) currentProg->stack_current);
    printf("stack_upward:   " FMT_U_MEM "\n", (memSizeType) currentProg->stack_current->upward);
    printf("stack_downward: " FMT_U_MEM "\n", (memSizeType) currentProg->stack_current->downward);
    */
    if (currentProg->stack_current != NULL) {
      close_current_stack(currentProg);
    } /* if */
  } /* close_stack */



void grow_stack (errInfoType *err_info)

  {
    stackType created_stack_element;

  /* grow_stack */
    logFunction(printf("grow_stack %d\n", data_depth););
    if (ALLOC_RECORD(created_stack_element, stackRecord, count.stack)) {
      /* printf("%lx grow_stack %d\n", created_stack_element, data_depth + 1); */
      created_stack_element->upward = NULL;
      created_stack_element->downward = prog->stack_data;
      created_stack_element->local_object_list = NULL;
      created_stack_element->object_list_insert_place =
          &created_stack_element->local_object_list;
      prog->stack_data->upward = created_stack_element;
      prog->stack_data = created_stack_element;
      data_depth++;
    } else {
      *err_info = MEMORY_ERROR;
    } /* if */
    logFunction(printf("grow_stack %d -->\n", data_depth););
  } /* grow_stack */



void shrink_stack (void)

  {
    listType list_element;
    stackType old_stack_element;

  /* shrink_stack */
    logFunction(printf("shrink_stack %d\n", data_depth););
    /* printf("%lx shrink_stack %d\n", prog->stack_data, data_depth); */
    list_element = prog->stack_data->local_object_list;
    while (list_element != NULL) {
      pop_object(prog, list_element->obj);
      list_element = list_element->next;
    } /* while */
    free_list(prog->stack_data->local_object_list);
    old_stack_element = prog->stack_data;
    prog->stack_data = prog->stack_data->downward;
    prog->stack_data->upward = NULL;
    FREE_RECORD(old_stack_element, stackRecord, count.stack);
    data_depth--;
    logFunction(printf("shrink_stack %d\n", data_depth););
  } /* shrink_stack */



void push_stack (void)

  { /* push_stack */
    logFunction(printf("push_stack %d\n", depth););
    if (prog->stack_current->upward != NULL) {
      /* printf("%lx push_stack %d\n", prog->stack_current->upward, depth + 1); */
      prog->stack_current = prog->stack_current->upward;
      depth++;
    } else {
      /* printf("cannot go up\n"); */
    } /* if */
    logFunction(printf("push_stack %d -->\n", depth););
  } /* push_stack */



void pop_stack (void)

  {
    listType list_element;

  /* pop_stack */
    logFunction(printf("pop_stack %d\n", depth););
    if (prog->stack_current->downward != NULL) {
      /* printf(FMT_U_MEM " pop_stack %d\n", (memSizeType) prog->stack_current, depth); */
      list_element = prog->stack_current->local_object_list;
      while (list_element != NULL) {
        pop_object(prog, list_element->obj);
        list_element = list_element->next;
      } /* while */
      free_list(prog->stack_current->local_object_list);
      prog->stack_current->local_object_list = NULL;
      prog->stack_current->object_list_insert_place =
          &prog->stack_current->local_object_list;
      prog->stack_current = prog->stack_current->downward;
      depth--;
    } else {
      /* printf("cannot go down\n"); */
    } /* if */
    logFunction(printf("pop_stack %d -->\n", depth););
  } /* pop_stack */



static void down_stack (void)

  { /* down_stack */
    logFunction(printf("down_stack %d\n", depth););
    if (prog->stack_current->downward != NULL) {
      /* printf("%lx down_stack %d\n", prog->stack_current, depth); */
      prog->stack_current = prog->stack_current->downward;
      depth--;
    } else {
      printf("cannot go down");
    } /* if */
    logFunction(printf("down_stack %d -->\n", depth););
  } /* down_stack */



listType *get_local_object_insert_place (void)

  { /* get_local_object_insert_place */
    return prog->stack_current->object_list_insert_place;
  } /* get_local_object_insert_place */



void pop_object_list (listType list_element)

  { /* pop_object_list */
    while (list_element != NULL) {
      pop_object(prog, list_element->obj);
      list_element = list_element->next;
    } /* while */
  } /* pop_object_list */



static void free_matched_list (listType matched_name_list)

  {
    listType name_elem;

  /* free_matched_list */
    logFunction(printf("free_matched_list\n"););
    name_elem = matched_name_list;
    while (name_elem != NULL) {
      if (CATEGORY_OF_OBJ(name_elem->obj) == MATCHOBJECT) {
        free_expression(name_elem->obj);
      } /* if */
      name_elem = name_elem->next;
    } /* while */
    free_list(matched_name_list);
    logFunction(printf("free_matched_list -->\n"););
  } /* free_matched_list */



static listType match_name_list (listType original_name_list, errInfoType *err_info)

  {
    listType name_elem;
    listType matched_name_list;
    listType *list_insert_place;
    objectType parameter;

  /* match_name_list */
    logFunction(printf("match_name_list\n"););
    name_elem = original_name_list;
    matched_name_list = NULL;
    list_insert_place = &matched_name_list;
    while (name_elem != NULL) {
      if (CATEGORY_OF_OBJ(name_elem->obj) == EXPROBJECT) {
        parameter = copy_expression(name_elem->obj, err_info);
        if (*err_info == OKAY_NO_ERROR) {
          if (match_expression(parameter) == NULL) {
            err_match(NO_MATCH, parameter);
            free_expression(parameter);
          } else {
            list_insert_place = append_element_to_list(list_insert_place,
                parameter, err_info);
          } /* if */
        } /* if */
      } else {
        if (HAS_ENTITY(name_elem->obj) &&
            GET_ENTITY(name_elem->obj)->syobject != NULL) {
          parameter = GET_ENTITY(name_elem->obj)->syobject;
        } else {
          parameter = name_elem->obj;
        } /* if */
        list_insert_place = append_element_to_list(list_insert_place,
            parameter, err_info);
      } /* if */
      name_elem = name_elem->next;
    } /* while */
    logFunction(printf("match_name_list -->\n"););
    return matched_name_list;
  } /* match_name_list */



static listType eval_name_list (listType matched_name_list,
    fileNumType file_number, lineNumType line, errInfoType *err_info)

  {
    listType name_elem;
    listType name_list;
    listType *list_insert_place;
    objectType parameter;

  /* eval_name_list */
    logFunction(printf("eval_name_list(" FMT_U_MEM ", %u, %u, %d)\n",
                       (memSizeType) matched_name_list,
                       file_number, line, *err_info););
    name_elem = matched_name_list;
    name_list = NULL;
    list_insert_place = &name_list;
    while (name_elem != NULL && *err_info == OKAY_NO_ERROR) {
      if (CATEGORY_OF_OBJ(name_elem->obj) == MATCHOBJECT) {
        if (name_elem->obj->type_of->result_type != take_type(SYS_F_PARAM_TYPE)) {
          err_object(PARAM_DECL_FAILED, name_elem->obj);
          *err_info = CREATE_ERROR;
        } else {
          parameter = do_exec_call(name_elem->obj, err_info);
          if (*err_info != OKAY_NO_ERROR) {
            err_object(PARAM_DECL_FAILED, name_elem->obj);
          } else if (CATEGORY_OF_OBJ(parameter) != FORMPARAMOBJECT) {
            err_object(PARAM_DECL_FAILED, name_elem->obj);
            *err_info = CREATE_ERROR;
          } else if (unlikely(parameter->value.objValue == NULL)) {
            FREE_OBJECT(parameter);
          } else {
            list_insert_place = append_element_to_list(list_insert_place,
                parameter, err_info);
          } /* if */
        } /* if */
      } else {
        if (HAS_ENTITY(name_elem->obj) &&
            GET_ENTITY(name_elem->obj)->syobject != NULL) {
          parameter = GET_ENTITY(name_elem->obj)->syobject;
        } else {
          parameter = name_elem->obj;
        } /* if */
        if (CATEGORY_OF_OBJ(parameter) != SYMBOLOBJECT) {
          err_at_file_in_line(PARAM_DECL_OR_SYMBOL_EXPECTED, parameter,
              file_number, line);
          *err_info = CREATE_ERROR;
        } else {
          list_insert_place = append_element_to_list(list_insert_place,
              parameter, err_info);
        } /* if */
      } /* if */
      name_elem = name_elem->next;
    } /* while */
    logFunction(printf("eval_name_list(" FMT_U_MEM ", %u, %u, %d) --> "
                       FMT_U_MEM "\n",
                       (memSizeType) matched_name_list,
                       file_number, line, *err_info,
                       (memSizeType) name_list););
    return name_list;
  } /* eval_name_list */



static objectType inst_list (nodeType declaration_base, const_objectType object_name,
    errInfoType *err_info)

  {
    listType matched_name_list;
    listType name_list;
    objectType defined_object;

  /* inst_list */
    logFunction(printf("inst_list(" FMT_U_MEM ", ",
                       (memSizeType) declaration_base);
                trace1(object_name);
                printf(", %d)\n", *err_info););
    matched_name_list = match_name_list(object_name->value.listValue, err_info);
    if (*err_info == OKAY_NO_ERROR) {
      push_stack();
      name_list = eval_name_list(matched_name_list,
          POSINFO_FILE_NUM(object_name), POSINFO_LINE_NUM(object_name), err_info);
      if (*err_info == OKAY_NO_ERROR) {
        down_stack();
        defined_object = push_name(prog, declaration_base, name_list,
            POSINFO_FILE_NUM(object_name), POSINFO_LINE_NUM(object_name), err_info);
      } else {
        pop_stack();
        free_name_list(name_list);
        defined_object = NULL;
      } /* if */
      free_matched_list(matched_name_list);
    } else {
      defined_object = NULL;
    } /* if */
    logFunction(printf("inst_list(" FMT_U_MEM ", ",
                       (memSizeType) declaration_base);
                trace1(object_name);
                printf(", %d) --> ", *err_info);
                trace1(defined_object);
                printf("\n"););
    return defined_object;
  } /* inst_list */



static objectType inst_object (const_nodeType declaration_base, objectType name_object,
    fileNumType file_number, lineNumType line, errInfoType *err_info)

  {
    objectType defined_object;

  /* inst_object */
    logFunction(printf("inst_object(");
                trace1(name_object);
                printf(", %d, %d, %d)\n", file_number, line, *err_info););
    if (name_object->descriptor.property == prog->property.literal) {
      err_object(IDENT_EXPECTED, name_object);
    } /* if */
    defined_object = get_object(prog, GET_ENTITY(name_object), NULL,
        file_number, line, err_info);
    logFunction(printf("inst_object --> ");
                trace1(defined_object);
                printf("\n"););
    return defined_object;
  } /* inst_object */



static objectType inst_object_expr (const_nodeType declaration_base,
    objectType object_name, errInfoType *err_info)

  {
    listType matched_name_list;
    listType name_list;
    objectType param_obj;
    objectType defined_object = NULL;

  /* inst_object_expr */
    logFunction(printf("inst_object_expr(" FMT_U_MEM ", ",
                       (memSizeType) declaration_base);
                trace1(object_name);
                printf(")\n"););
    matched_name_list = match_name_list(object_name->value.listValue, err_info);
    if (*err_info == OKAY_NO_ERROR) {
      push_stack();
      name_list = eval_name_list(matched_name_list,
          POSINFO_FILE_NUM(object_name), POSINFO_LINE_NUM(object_name), err_info);
      if (*err_info == OKAY_NO_ERROR) {
        down_stack();
        /* printf("name_list ");
        prot_list(name_list);
        printf("\n");
        fflush(stdout);
        printf("name_list->obj ");
        trace1(name_list->obj);
        printf("\n");
        fflush(stdout); */
        if (CATEGORY_OF_OBJ(name_list->obj) == FORMPARAMOBJECT) {
          param_obj = name_list->obj->value.objValue;
          if (CATEGORY_OF_OBJ(param_obj) == VALUEPARAMOBJECT ||
              CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT ||
              CATEGORY_OF_OBJ(param_obj) == TYPEOBJECT) {
            err_object(IDENT_EXPECTED, object_name);
          } else {
            /* printf("param_obj ");
            trace1(param_obj);
            printf("\n"); */
            if (HAS_PROPERTY(param_obj)) {
              defined_object = inst_object(declaration_base, param_obj,
                  PROPERTY_FILE_NUM(param_obj),
                  PROPERTY_LINE_NUM(param_obj), err_info);
            } else {
              defined_object = inst_object(declaration_base, param_obj,
                  0, 0, err_info);
            } /* if */
          } /* if */
        } else {
          err_object(IDENT_EXPECTED, object_name);
        } /* if */
        free_form_param_list(name_list);
      } else {
        pop_stack();
        free_name_list(name_list);
      } /* if */
      free_matched_list(matched_name_list);
    } /* if */
    logFunction(printf("inst_object_expr --> ");
                trace1(defined_object);
                printf("\n"););
    return defined_object;
  } /* inst_object_expr */



objectType entername (nodeType declaration_base, objectType object_name,
    errInfoType *err_info)

  {
    objectType defined_object;

  /* entername */
    logFunction(printf("entername(" FMT_U_MEM ", ",
                       (memSizeType) declaration_base);
                trace1(object_name);
                printf(", %d)\n", *err_info););
    if (CATEGORY_OF_OBJ(object_name) == EXPROBJECT) {
      if (object_name->value.listValue->next != NULL) {
        defined_object = inst_list(declaration_base, object_name, err_info);
      } else if (CATEGORY_OF_OBJ(object_name->value.listValue->obj) == EXPROBJECT ||
          CATEGORY_OF_OBJ(object_name->value.listValue->obj) == MATCHOBJECT) {
        defined_object = inst_object_expr(declaration_base, object_name, err_info);
      } else {
        /* printf("listValue->obj ");
        trace1(object_name->value.listValue->obj);
        printf(")\n"); */
        defined_object = inst_object(declaration_base,
            object_name->value.listValue->obj,
            POSINFO_FILE_NUM(object_name), POSINFO_LINE_NUM(object_name), err_info);
      } /* if */
    } else {
      defined_object = inst_object(declaration_base, object_name, 0, 0, err_info);
    } /* if */
    /* trace_nodes(); */
    logFunction(printf("entername(" FMT_U_MEM ", ",
                       (memSizeType) declaration_base);
                trace1(object_name);
                printf(", %d) --> ", *err_info);
                trace1(defined_object);
                printf("\n"););
    return defined_object;
  } /* entername */



objectType find_name (nodeType declaration_base, const_objectType object_name,
    errInfoType *err_info)

  {
    listType matched_name_list;
    listType name_list;
    objectType param_obj;
    entityType entity;
    objectType defined_object;

  /* find_name */
    logFunction(printf("find_name(" FMT_U_MEM ", ",
                       (memSizeType) declaration_base);
                trace1(object_name);
                printf(")\n"););
    if (CATEGORY_OF_OBJ(object_name) == EXPROBJECT) {
      grow_stack(err_info);
      if (*err_info == OKAY_NO_ERROR) {
        if (object_name->value.listValue->next != NULL) {
          matched_name_list = match_name_list(object_name->value.listValue, err_info);
          if (*err_info == OKAY_NO_ERROR) {
            push_stack();
            name_list = eval_name_list(matched_name_list,
                POSINFO_FILE_NUM(object_name), POSINFO_LINE_NUM(object_name), err_info);
            pop_stack();
            if (*err_info == OKAY_NO_ERROR) {
              entity = find_entity(declaration_base, name_list);
            } else {
              entity = NULL;
            } /* if */
            shrink_stack();
            free_name_list(name_list);
          } else {
            shrink_stack();
            entity = NULL;
          } /* if */
          free_matched_list(matched_name_list);
        } else if (CATEGORY_OF_OBJ(object_name->value.listValue->obj) == EXPROBJECT ||
            CATEGORY_OF_OBJ(object_name->value.listValue->obj) == MATCHOBJECT) {
          matched_name_list = match_name_list(object_name->value.listValue, err_info);
          if (*err_info == OKAY_NO_ERROR) {
            push_stack();
            name_list = eval_name_list(matched_name_list,
                POSINFO_FILE_NUM(object_name), POSINFO_LINE_NUM(object_name), err_info);
            pop_stack();
            if (*err_info == OKAY_NO_ERROR) {
              if (CATEGORY_OF_OBJ(name_list->obj) == FORMPARAMOBJECT) {
                param_obj = name_list->obj->value.objValue;
                if (CATEGORY_OF_OBJ(param_obj) == VALUEPARAMOBJECT ||
                    CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT ||
                    CATEGORY_OF_OBJ(param_obj) == TYPEOBJECT) {
                  entity = NULL;
                } else {
                  entity = GET_ENTITY(param_obj);
                } /* if */
              } else {
                entity = NULL;
              } /* if */
            } else {
              entity = NULL;
            } /* if */
            shrink_stack();
            free_name_list(name_list);
          } else {
            shrink_stack();
            entity = NULL;
          } /* if */
          free_matched_list(matched_name_list);
        } else {
          entity = GET_ENTITY(object_name->value.listValue->obj);
          shrink_stack();
        } /* if */
      } else {
        entity = NULL;
      } /* if */
    } else {
      entity = GET_ENTITY(object_name);
    } /* if */
    if (entity != NULL && entity->data.owner != NULL) {
      defined_object = entity->data.owner->obj;
    } else {
      defined_object = NULL;
    } /* if */
    /* trace_nodes(); */
    logFunction(printf("find_name(");
                trace1(object_name);
                printf(") --> ");
                trace1(defined_object);
                printf("\n"););
    return defined_object;
  } /* find_name */



objectType search_name (const_nodeType declaration_base,
    const_objectType object_name, errInfoType *err_info)

  {
    listType matched_name_list;
    listType name_list;
    objectType param_obj;
    entityType entity;
    objectType defined_object;

  /* search_name */
    logFunction(printf("search_name(" FMT_U_MEM ", ",
                       (memSizeType) declaration_base);
                trace1(object_name);
                printf(")\n"););
    if (CATEGORY_OF_OBJ(object_name) == EXPROBJECT) {
      grow_stack(err_info);
      if (*err_info == OKAY_NO_ERROR) {
        if (object_name->value.listValue->next != NULL) {
          matched_name_list = match_name_list(object_name->value.listValue, err_info);
          if (*err_info == OKAY_NO_ERROR) {
            push_stack();
            name_list = eval_name_list(matched_name_list,
                POSINFO_FILE_NUM(object_name), POSINFO_LINE_NUM(object_name), err_info);
            pop_stack();
            if (*err_info == OKAY_NO_ERROR) {
              entity = search_entity(declaration_base, name_list);
            } else {
              entity = NULL;
            } /* if */
            shrink_stack();
            free_name_list(name_list);
          } else {
            shrink_stack();
            entity = NULL;
          } /* if */
          free_matched_list(matched_name_list);
        } else if (CATEGORY_OF_OBJ(object_name->value.listValue->obj) == EXPROBJECT ||
            CATEGORY_OF_OBJ(object_name->value.listValue->obj) == MATCHOBJECT) {
          matched_name_list = match_name_list(object_name->value.listValue, err_info);
          if (*err_info == OKAY_NO_ERROR) {
            push_stack();
            name_list = eval_name_list(matched_name_list,
                POSINFO_FILE_NUM(object_name), POSINFO_LINE_NUM(object_name), err_info);
            pop_stack();
            if (*err_info == OKAY_NO_ERROR) {
              if (CATEGORY_OF_OBJ(name_list->obj) == FORMPARAMOBJECT) {
                param_obj = name_list->obj->value.objValue;
                if (CATEGORY_OF_OBJ(param_obj) == VALUEPARAMOBJECT ||
                    CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT ||
                    CATEGORY_OF_OBJ(param_obj) == TYPEOBJECT) {
                  entity = NULL;
                } else {
                  entity = GET_ENTITY(param_obj);
                } /* if */
              } else {
                entity = NULL;
              } /* if */
            } else {
              entity = NULL;
            } /* if */
            shrink_stack();
            free_name_list(name_list);
          } else {
            shrink_stack();
            entity = NULL;
          } /* if */
          free_matched_list(matched_name_list);
        } else {
          entity = GET_ENTITY(object_name->value.listValue->obj);
          shrink_stack();
        } /* if */
      } else {
        entity = NULL;
      } /* if */
    } else {
      entity = GET_ENTITY(object_name);
    } /* if */
    if (entity != NULL && entity->data.owner != NULL) {
      defined_object = entity->data.owner->obj;
    } else {
      defined_object = NULL;
    } /* if */
    /* trace_nodes(); */
    logFunction(printf("search_name(");
                trace1(object_name);
                printf(") --> ");
                trace1(defined_object);
                printf("\n"););
    return defined_object;
  } /* search_name */



static objectType eval_type (objectType object)

  {
    objectType matched_expression;
    errInfoType err_info = OKAY_NO_ERROR;
    objectType result;

  /* eval_type */
    logFunction(printf("eval_type(");
                trace1(object);
                printf(")\n"););
    matched_expression = copy_expression(object, &err_info);
    if (err_info == OKAY_NO_ERROR) {
      if (match_expression(matched_expression) != NULL) {
        if (CATEGORY_OF_OBJ(matched_expression) == MATCHOBJECT) {
          if (!matched_expression->type_of->result_type->is_type_type) {
            err_object(TYPE_EXPECTED, matched_expression);
            result = NULL;
          } else {
            result = do_exec_call(matched_expression, &err_info);
            if (err_info != OKAY_NO_ERROR) {
              err_object(EXCEPTION_RAISED, result);
              result = NULL;
            } else if (CATEGORY_OF_OBJ(result) != TYPEOBJECT) {
              err_object(PARAM_DECL_FAILED, result);
              result = NULL;
            } else {
              free_list(object->value.listValue);
              FREE_OBJECT(object);
            } /* if */
          } /* if */
        } else {
          err_match(NO_MATCH, object);
          result = NULL;
        } /* if */
      } else {
        err_match(NO_MATCH, object);
        result = NULL;
      } /* if */
      free_expression(matched_expression);
    } else {
      result = NULL;
    } /* if */
    logFunction(printf("eval_type --> ");
                trace1(result);
                printf("\n"););
    return result;
  } /* eval_type */



static objectType dollar_parameter (objectType param_object,
    errInfoType *err_info)

  {
    listType param_descr;
    objectType param_object_copy = NULL;
    objectType type_of_parameter;

  /* dollar_parameter */
    logFunction(printf("dollar_parameter(");
                trace1(param_object);
                printf(", %d)\n", *err_info););
    param_object_copy = copy_expression(param_object, err_info);
    if (*err_info == OKAY_NO_ERROR) {
      param_descr = param_object_copy->value.listValue;
      if (param_descr != NULL) {
        if (GET_ENTITY(param_descr->obj)->ident == prog->id_for.ref) {
          /* printf("### ref param\n"); */
          if (param_descr->next != NULL && param_descr->next->obj != NULL) {
            type_of_parameter = param_descr->next->obj;
            if (CATEGORY_OF_OBJ(type_of_parameter) == EXPROBJECT) {
              type_of_parameter = eval_type(type_of_parameter);
              if (unlikely(type_of_parameter == NULL)) {
                param_object = NULL;
              } else {
                param_descr->next->obj = type_of_parameter;
              } /* if */
            } /* if */
            if (type_of_parameter != NULL) {
              if (CATEGORY_OF_OBJ(type_of_parameter) == TYPEOBJECT) {
                if (param_descr->next->next != NULL) {
                  if (GET_ENTITY(param_descr->next->next->obj)->ident == prog->id_for.colon) {
                    param_object = dcl_ref2(param_descr);
                  } else {
                    param_object = dcl_ref1(param_descr);
                  } /* if */
                  if (param_object == NULL) {
                    *err_info = MEMORY_ERROR;
                  } /* if */
                } /* if */
              } else {
                err_object(TYPE_EXPECTED, type_of_parameter);
                param_object = NULL;
              } /* if */
            } /* if */
          } /* if */
        } else {
          err_ident(PARAM_SPECIFIER_EXPECTED, GET_ENTITY(param_descr->obj)->ident);
          param_object = NULL;
        } /* if */
      } /* if */
      free_expression(param_object_copy);
    } /* if */
    logFunction(printf("dollar_parameter(*, %d) --> ", *err_info);
                trace1(param_object);
                printf("\n"););
    return param_object;
  } /* dollar_parameter */



static objectType dollar_inst_list (nodeType declaration_base,
    const_objectType object_name, errInfoType *err_info)

  {
    listType name_elem;
    listType name_list;
    listType *list_insert_place;
    boolType parsing_error = FALSE;
    objectType parameter;
    objectType defined_object;

  /* dollar_inst_list */
    logFunction(printf("dollar_inst_list(" FMT_U_MEM ", ",
                       (memSizeType) declaration_base);
                trace1(object_name);
                printf(", %d)\n", *err_info););
    name_elem = object_name->value.listValue;
    name_list = NULL;
    list_insert_place = &name_list;
    while (name_elem != NULL) {
      if (CATEGORY_OF_OBJ(name_elem->obj) == EXPROBJECT) {
        parameter = dollar_parameter(name_elem->obj, err_info);
      } else {
        parameter = name_elem->obj;
      } /* if */
      if (parameter == NULL) {
        parsing_error = TRUE;
      } else {
        list_insert_place = append_element_to_list(list_insert_place,
            parameter, err_info);
      } /* if */
      name_elem = name_elem->next;
    } /* while */
    if (!parsing_error && *err_info == OKAY_NO_ERROR) {
      defined_object = push_name(prog, declaration_base,
          name_list, POSINFO_FILE_NUM(object_name),
          POSINFO_LINE_NUM(object_name), err_info);
    } else {
      free_list(name_list);
      defined_object = NULL;
    } /* if */
    logFunction(printf("dollar_inst_list(" FMT_U_MEM ", ",
                       (memSizeType) declaration_base);
                trace1(object_name);
                printf(", %d) -->", *err_info);
                trace1(defined_object);
                printf("\n"););
    return defined_object;
  } /* dollar_inst_list */



objectType dollar_entername (nodeType declaration_base, objectType object_name,
    errInfoType *err_info)

  {
    objectType defined_object;

  /* dollar_entername */
    logFunction(printf("dollar_entername(" FMT_U_MEM ", ",
                       (memSizeType) declaration_base);
                trace1(object_name);
                printf(", %d)\n", *err_info););
    if (CATEGORY_OF_OBJ(object_name) == EXPROBJECT) {
      defined_object = dollar_inst_list(declaration_base, object_name, err_info);
    } else {
      defined_object = inst_object(declaration_base, object_name, 0, 0, err_info);
    } /* if */
    /* trace_nodes(); */
    logFunction(printf("dollar_entername(");
                trace1(object_name);
                printf(", %d) --> ", *err_info);
                trace1(defined_object);
                printf("\n"););
    return defined_object;
  } /* dollar_entername */
