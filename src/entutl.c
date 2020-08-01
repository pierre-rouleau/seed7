/********************************************************************/
/*                                                                  */
/*  s7   Seed7 interpreter                                          */
/*  Copyright (C) 1990 - 2013  Thomas Mertes                        */
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
/*  Module: General                                                 */
/*  File: seed7/src/entutl.c                                        */
/*  Changes: 2000  Thomas Mertes                                    */
/*  Content: Procedures to maintain objects of type entityType.     */
/*                                                                  */
/********************************************************************/

#include "version.h"

#include "stdlib.h"
#include "stdio.h"

#include "common.h"
#include "data.h"
#include "heaputl.h"
#include "flistutl.h"
#include "listutl.h"
#include "datautl.h"
#include "traceutl.h"

#undef EXTERN
#define EXTERN
#include "entutl.h"

#undef TRACE_ENTITY


  /* The macro PTR_LESS is used to generate a less than comparison  */
  /* for pointers. Since the comparison is used to build a binary   */
  /* tree a true less operation would have the danger of            */
  /* degenerating the tree to a linear list. But a true less        */
  /* comparison works since I have used it for years at this place. */
  /* If you want a true less comparison replace this macro by:      */
  /*                                                                */
  /*   #define PTR_LESS(P1,P2) ((P1) < (P2))                        */
  /*                                                                */
  /* The K&R standard allowes pointer comparisons only inside of    */
  /* arrays. So the true less comparison does not conform to the    */
  /* standard. The macro below converts the pointers to long so it  */
  /* is confoming to the standard. Taking only the lower bits for   */
  /* the comparison has a "random" effect for the binary tree.      */
  /* This speeds the tree up a measurable amount of time.           */

#define PTR_LESS(P1,P2) (((long) (P1) & 0377L) < ((long) (P2) & 0377L))

/* #define PTR_LESS(P1,P2) (((long) (P1) & 0177400L) < ((long) (P2) & 0177400L)) */
/* #define PTR_LESS(P1,P2) ((P1) < (P2)) */



static void free_nodes (nodeType node)

  { /* free_nodes */
#ifdef TRACE_ENTITY
    printf("BEGIN free_nodes(");
    printnodes(node);
    printf(")\n");
#endif
    if (node != NULL) {
      /* node->entity is not removed here */
      free_nodes(node->next1);
      free_nodes(node->next2);
      free_nodes(node->symbol);
      free_nodes(node->inout_param);
      free_nodes(node->other_param);
      free_nodes(node->attr);
      FREE_NODE(node);
    } /* if */
#ifdef TRACE_ENTITY
    printf("END free_nodes\n");
#endif
  } /* free_nodes */



static nodeType new_node (objectType obj)

  {
    nodeType created_node;

  /* new_node */
#ifdef TRACE_ENTITY
    printf("BEGIN new_node(");
    trace1(obj);
    printf(")\n");
#endif
    if (ALLOC_NODE(created_node)) {
      created_node->usage_count = 1;
      created_node->match_obj = obj;
      created_node->next1 = NULL;
      created_node->next2 = NULL;
      created_node->entity = NULL;
      created_node->symbol = NULL;
      created_node->inout_param = NULL;
      created_node->other_param = NULL;
      created_node->attr = NULL;
    } /* if */
#ifdef TRACE_ENTITY
    printf("END new_node --> ");
    trace_node(created_node);
    printf("\n");
#endif
    return created_node;
  } /* new_node */



/**
 *  Searches node_tree for a node matching object_searched.
 *  If a node is found it is returned. If no matching node is
 *  found a new node is generated and entered into node_tree.
 *  In this case the new node is returned.
 *  @return the node found or the new generated node or
 *          NULL, if a memory error occoured.
 */
static nodeType get_node (nodeType *node_tree,
    register objectType object_searched)

  {
    register nodeType search_node;
    register boolType searching;
    nodeType node_found;

  /* get_node */
#ifdef TRACE_ENTITY
    printf("BEGIN get_node({ ");
    printnodes(*node_tree);
    printf("}, ");
    trace1(object_searched);
    printf(")\n");
#endif
    if ((search_node = *node_tree) == NULL) {
      node_found = new_node(object_searched);
      *node_tree = node_found;
    } else {
      node_found = NULL;
      searching = TRUE;
      do {
        if (PTR_LESS(object_searched, search_node->match_obj)) {
          if (search_node->next1 == NULL) {
            node_found = new_node(object_searched);
            search_node->next1 = node_found;
            searching = FALSE;
          } else {
            search_node = search_node->next1;
          } /* if */
        } else if (object_searched == search_node->match_obj) {
          node_found = search_node;
          node_found->usage_count++;
          searching = FALSE;
        } else {    /* (object_searched > search_node->match_obj) */
          if (search_node->next2 == NULL) {
            node_found = new_node(object_searched);
            search_node->next2 = node_found;
            searching = FALSE;
          } else {
            search_node = search_node->next2;
          } /* if */
        } /* if */
      } while (searching);
    } /* if */
/* printf("get_node >%s<\n", object_searched->entity->ident->name); */
#ifdef TRACE_ENTITY
    printf("END get_node --> ");
    trace_node(node_found);
    printf("\n");
#endif
    return node_found;
  } /* get_node */



/**
 *  Searches node_tree for a node matching object_searched.
 *  @return the node found or NULL if no matching node is found.
 */
nodeType find_node (register nodeType node_tree,
    register objectType object_searched)

  {
    nodeType node_found;

  /* find_node */
#ifdef TRACE_ENTITY
    printf("BEGIN find_node({ ");
    printnodes(node_tree);
    printf("}, ");
    trace1(object_searched);
    prot_cstri("=");
    prot_int((intType) object_searched);
    printf(")\n");
#endif
    node_found = NULL;
    while (node_tree != NULL) {
      if (PTR_LESS(object_searched, node_tree->match_obj)) {
        node_tree = node_tree->next1;
      } else if (object_searched == node_tree->match_obj) {
        if (node_tree->usage_count > 0) {
          node_found = node_tree;
        } /* if */
        node_tree = NULL;
      } else {    /* (object_searched > node_tree->match_obj) */
        node_tree = node_tree->next2;
      } /* if */
    } /* while */
/*  printf("%s\n", (node_found != NULL ? "found" : "not found")); */
#ifdef TRACE_ENTITY
    printf("END find_node --> ");
    if (node_found != NULL) {
      trace1(node_found->match_obj);
      printf("\n");
    } else {
      printf("*NULL_NODE*\n");
    } /* if */
#endif
    return node_found;
  } /* find_node */



/**
 *  Decrement the usage count of object_searched, when it is found.
 *  Searches node_tree for a node matching object_searched.
 *  If a node is found its usage_count is decremented. If the
 *  node is unused now it is removed and NULL is returned.
 *  If the node is still in use it is returned. If no matching
 *  node is found NULL is returned.
 *  @return a node that is still in use after it has been popped, or
 *          NULL when the last usage of a node has been removed, or
 *          NULL when no matching node has been found.
 */
static nodeType pop_node (register nodeType node_tree,
    register objectType object_searched)

  {
    nodeType node_found;

  /* pop_node */
#ifdef TRACE_ENTITY
    printf("BEGIN pop_node({ ");
    printnodes(node_tree);
    printf("}, ");
    trace1(object_searched);
    prot_cstri("=");
    prot_int((intType) object_searched);
    printf(")\n");
#endif
    node_found = NULL;
    while (node_tree != NULL) {
      if (PTR_LESS(object_searched, node_tree->match_obj)) {
        node_tree = node_tree->next1;
      } else if (object_searched == node_tree->match_obj) {
        if (node_tree->usage_count > 0) {
          node_tree->usage_count--;
          if (node_tree->usage_count == 0) {
            node_tree->entity = NULL;
            free_nodes(node_tree->symbol);
            free_nodes(node_tree->inout_param);
            free_nodes(node_tree->other_param);
            free_nodes(node_tree->attr);
            node_tree->symbol = NULL;
            node_tree->inout_param = NULL;
            node_tree->other_param = NULL;
            node_tree->attr = NULL;
          } else {
            node_found = node_tree;
          } /* if */
        } /* if */
        node_tree = NULL;
      } else {    /* (object_searched > node_tree->match_obj) */
        node_tree = node_tree->next2;
      } /* if */
    } /* while */
/*  printf("%s\n", (node_found != NULL ? "found" : "not found")); */
#ifdef TRACE_ENTITY
    printf("END pop_node --> ");
    if (node_found != NULL) {
      trace1(node_found->match_obj);
      printf("\n");
    } else {
      printf("*NULL_NODE*\n");
    } /* if */
#endif
    return node_found;
  } /* pop_node */



void init_declaration_root (progType currentProg, errInfoType *err_info)

  { /* init_declaration_root */
#ifdef TRACE_ENTITY
    printf("BEGIN init_declaration_root\n");
#endif
    currentProg->declaration_root = new_node(NULL);
#ifdef TRACE_ENTITY
    printf("END init_declaration_root\n");
#endif
  } /* init_declaration_root */



void close_declaration_root (progType currentProg)

  { /* close_declaration_root */
#ifdef TRACE_ENTITY
    printf("BEGIN close_declaration_root\n");
#endif
    free_nodes(currentProg->declaration_root);
    currentProg->declaration_root = NULL;
#ifdef TRACE_ENTITY
    printf("END close_declaration_root\n");
#endif
  } /* close_declaration_root */



void free_entity (const_progType currentProg, entityType old_entity)

  {
    listType name_elem;
    objectType param_obj;

  /* free_entity */
#ifdef TRACE_ENTITY
    printf("BEGIN free_entity\n");
#endif
    if (old_entity != NULL) {
      if (old_entity->syobject != NULL) {
        if (HAS_PROPERTY(old_entity->syobject) && old_entity->syobject->descriptor.property != currentProg->property.literal) {
          /* trace1(old_entity->syobject);
             printf("\n"); */
          FREE_RECORD(old_entity->syobject->descriptor.property, propertyRecord, count.property);
        } /* if */
        FREE_OBJECT(old_entity->syobject);
      } /* if */
      if (old_entity->fparam_list != NULL) {
        name_elem = old_entity->fparam_list;
        while (name_elem != NULL) {
          if (CATEGORY_OF_OBJ(name_elem->obj) == FORMPARAMOBJECT) {
            param_obj = name_elem->obj->value.objValue;
            if (CATEGORY_OF_OBJ(param_obj) == VALUEPARAMOBJECT ||
                CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT) {
              FREE_OBJECT(param_obj);
            } /* if */
            FREE_OBJECT(name_elem->obj);
          } /* if */
          name_elem = name_elem->next;
        } /* if */
        free_list(old_entity->fparam_list);
      } /* if */
      FREE_RECORD(old_entity, entityRecord, count.entity);
    } /* if */
#ifdef TRACE_ENTITY
    printf("END free_entity\n");
#endif
  } /* free_entity */



static entityType new_entity (identType id)

  {
    entityType created_entity;

  /* new_entity */
#ifdef TRACE_ENTITY
    printf("BEGIN new_entity\n");
#endif
    if (ALLOC_RECORD(created_entity, entityRecord, count.entity)) {
      created_entity->ident = id;
      created_entity->syobject = NULL;
      created_entity->fparam_list = NULL;
      created_entity->data.owner = NULL;
    } /* if */
#ifdef TRACE_ENTITY
    printf("END new_entity --> ");
    prot_cstri(id_string(created_entity->ident));
    printf("\n");
#endif
    return created_entity;
  } /* new_entity */



static listType copy_params (listType name_list)

  {
    listType name_elem;
    objectType param_obj;
    objectType copied_param;

  /* copy_params */
    name_elem = name_list;
    while (name_elem != NULL) {
      if (CATEGORY_OF_OBJ(name_elem->obj) == FORMPARAMOBJECT) {
        param_obj = name_elem->obj->value.objValue;
        if (CATEGORY_OF_OBJ(param_obj) == VALUEPARAMOBJECT ||
            CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT) {
          /* printf("copy_params: ");
          trace1(param_obj);
          printf("\n"); */
          if (ALLOC_OBJECT(copied_param)) {
            INIT_CATEGORY_OF_OBJ(copied_param, CATEGORY_OF_OBJ(param_obj));
            COPY_VAR_FLAG(copied_param, param_obj);
            copied_param->type_of = param_obj->type_of;
            copied_param->descriptor.property = NULL;
            copied_param->value.objValue = NULL;
            /* memcpy(copied_param, param_obj, sizeof(objectRecord)); */
            name_elem->obj->value.objValue = copied_param;
          } else {
            name_list = NULL;
          } /* if */
        } /* if */
      } /* if */
      name_elem = name_elem->next;
    } /* while */
    return name_list;
  } /* copy_params */



/**
 *  Searches declaration_base for an entity matching name_list.
 *  If an entity is found it is returned. If no matching entity is
 *  found a new entity is generated and entered into declaration_base.
 *  In this case the new entity is returned.
 *  @return the entity found or the new generated entity or
 *          NULL, if a memory error occoured.
 */
entityType get_entity (nodeType declaration_base, listType name_list)

  {
    listType name_elem;
    objectType param_obj;
    nodeType curr_node;
    entityType entity_found;

  /* get_entity */
#ifdef TRACE_ENTITY
    printf("BEGIN get_entity(");
    prot_list(name_list);
    printf(")\n");
#endif
    name_elem = name_list;
    curr_node = declaration_base;
    while (name_elem != NULL && curr_node != NULL) {
      if (CATEGORY_OF_OBJ(name_elem->obj) == FORMPARAMOBJECT) {
/* printf("paramobject x\n"); */
        param_obj = name_elem->obj->value.objValue;
        if (CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT && VAR_OBJECT(param_obj)) {
/* printf("inout param ");
trace1(param_obj);
printf("\n"); */
          curr_node = get_node(&curr_node->inout_param, param_obj->type_of->match_obj);
        } else if (CATEGORY_OF_OBJ(param_obj) == VALUEPARAMOBJECT ||
            CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT) {
/* printf("value or ref param ");
trace1(param_obj);
printf("\n"); */
          curr_node = get_node(&curr_node->other_param, param_obj->type_of->match_obj);
        } else if (CATEGORY_OF_OBJ(param_obj) == TYPEOBJECT) {
/* printf("attr param ");
trace1(param_obj);
printf("\n"); */
          curr_node = get_node(&curr_node->attr, param_obj->value.typeValue->match_obj);
        } else if (CATEGORY_OF_OBJ(param_obj) == FORMPARAMOBJECT) {
/* printf("attr param attr ");
trace1(param_obj);
printf("\n"); */
          curr_node = get_node(&curr_node->attr, param_obj);
        } else {
/* printf("symbol param ");
trace1(param_obj);
printf("\n"); */
          curr_node = get_node(&curr_node->symbol, param_obj);
        } /* if */
      } else {
/* printf("symbol param ");
trace1(name_elem->obj);
printf("\n"); */
        curr_node = get_node(&curr_node->symbol, name_elem->obj);
/* printf("after symbol param\n"); */
      } /* if */
      name_elem = name_elem->next;
    } /* while */
    if (curr_node == NULL) {
      entity_found = NULL;
    } else {
      if (curr_node->entity == NULL) {
        if ((entity_found = new_entity(NULL)) != NULL) {
          entity_found->fparam_list = copy_params(name_list);
          if (entity_found->fparam_list == NULL) {
            FREE_RECORD(entity_found, entityRecord, count.entity);
            entity_found = NULL;
          } else {
            curr_node->entity = entity_found;
          } /* if */
        } /* if */
      } else {
        entity_found = curr_node->entity;
      } /* if */
    } /* if */
#ifdef TRACE_ENTITY
    printf("END get_entity -->\n");
    trace_entity(entity_found);
#endif
    return entity_found;
  } /* get_entity */



/**
 *  Searches declaration_base for an entity matching name_list.
 *  @return the entity found or NULL if no matching entity is found.
 */
entityType find_entity (nodeType declaration_base, listType name_list)

  {
    listType name_elem;
    objectType param_obj;
    nodeType curr_node;
    entityType entity_found;

  /* find_entity */
#ifdef TRACE_ENTITY
    printf("BEGIN find_entity(");
    prot_list(name_list);
    printf(")\n");
#endif
    name_elem = name_list;
    curr_node = declaration_base;
    while (name_elem != NULL && curr_node != NULL) {
      if (CATEGORY_OF_OBJ(name_elem->obj) == FORMPARAMOBJECT) {
/* printf("paramobject x\n"); */
        param_obj = name_elem->obj->value.objValue;
        if (CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT && VAR_OBJECT(param_obj)) {
/* printf("inout param ");
trace1(param_obj);
printf("\n"); */
          curr_node = find_node(curr_node->inout_param, param_obj->type_of->match_obj);
        } else if (CATEGORY_OF_OBJ(param_obj) == VALUEPARAMOBJECT ||
            CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT) {
/* printf("value or ref param ");
trace1(param_obj);
printf("\n"); */
          curr_node = find_node(curr_node->other_param, param_obj->type_of->match_obj);
        } else if (CATEGORY_OF_OBJ(param_obj) == TYPEOBJECT) {
/* printf("attr param ");
trace1(param_obj);
printf("\n"); */
          curr_node = find_node(curr_node->attr, param_obj->value.typeValue->match_obj);
        } else if (CATEGORY_OF_OBJ(param_obj) == FORMPARAMOBJECT) {
/* printf("attr param attr ");
trace1(param_obj);
printf("\n"); */
          curr_node = find_node(curr_node->attr, param_obj);
        } else {
/* printf("symbol param ");
trace1(param_obj);
printf("\n"); */
          curr_node = find_node(curr_node->symbol, param_obj);
        } /* if */
      } else {
/* printf("symbol param ");
trace1(name_elem->obj);
printf("\n"); */
        curr_node = find_node(curr_node->symbol, name_elem->obj);
/* printf("after symbol param\n"); */
      } /* if */
      name_elem = name_elem->next;
    } /* while */
    if (curr_node != NULL) {
      entity_found = curr_node->entity;
    } else {
      entity_found = NULL;
    } /* if */
#ifdef TRACE_ENTITY
    printf("END find_entity -->\n");
    trace_entity(entity_found);
#endif
    return entity_found;
  } /* find_entity */



entityType search_entity (const_nodeType start_node, const_listType name_list)

  {
    objectType param_obj;
    nodeType node_found;
    typeType object_type;
    entityType entity_found;

  /* search_entity */
#ifdef TRACE_ENTITY
    printf("BEGIN search_entity(");
    prot_list(name_list);
    printf(")\n");
#endif
    if (name_list == NULL) {
      if (start_node != NULL) {
        entity_found = start_node->entity;
      } else {
        entity_found = NULL;
      } /* if */
    } else {
      entity_found = NULL;
      if (start_node != NULL) {
        if (CATEGORY_OF_OBJ(name_list->obj) == FORMPARAMOBJECT) {
/* printf("paramobject x\n"); */
          param_obj = name_list->obj->value.objValue;
          if (CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT && VAR_OBJECT(param_obj)) {
/* printf("inout param ");
trace1(param_obj);
printf("\n"); */
            object_type = param_obj->type_of;
            do {
              node_found = find_node(start_node->inout_param, object_type->match_obj);
              if (node_found != NULL) {
                entity_found = search_entity(node_found, name_list->next);
              } /* if */
              object_type = object_type->meta;
            } while (object_type != NULL && entity_found == NULL);
          } else if (CATEGORY_OF_OBJ(param_obj) == VALUEPARAMOBJECT ||
              CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT) {
/* printf("value or ref param ");
trace1(param_obj);
printf("\n"); */
            object_type = param_obj->type_of;
            do {
              node_found = find_node(start_node->other_param, object_type->match_obj);
              if (node_found != NULL) {
                entity_found = search_entity(node_found, name_list->next);
              } /* if */
              object_type = object_type->meta;
            } while (object_type != NULL && entity_found == NULL);
          } else if (CATEGORY_OF_OBJ(param_obj) == TYPEOBJECT) {
/* printf("attr param ");
trace1(param_obj);
printf("\n"); */
            object_type = param_obj->value.typeValue;
            do {
              node_found = find_node(start_node->attr, object_type->match_obj);
              if (node_found != NULL) {
                entity_found = search_entity(node_found, name_list->next);
              } /* if */
              object_type = object_type->meta;
            } while (object_type != NULL && entity_found == NULL);
          } else {
/* printf("symbol param ");
trace1(param_obj);
printf("\n"); */
            node_found = find_node(start_node->symbol, param_obj);
            if (node_found != NULL) {
              entity_found = search_entity(node_found, name_list->next);
            } /* if */
          } /* if */
        } else {
/* printf("symbol param ");
trace1(name_list->obj);
printf("\n"); */
          node_found = find_node(start_node->symbol, name_list->obj);
          if (node_found != NULL) {
            entity_found = search_entity(node_found, name_list->next);
          } /* if */
/* printf("after symbol param\n"); */
        } /* if */
      } /* if */
    } /* if */
#ifdef TRACE_ENTITY
    printf("END search_entity\n");
#endif
    return entity_found;
  } /* search_entity */



void pop_entity (nodeType declaration_base, const_entityType entity)

  {
    listType name_elem;
    objectType param_obj;
    nodeType curr_node;

  /* pop_entity */
#ifdef TRACE_ENTITY
    printf("BEGIN pop_entity\n");
#endif
    /* trace_entity(entity); */
    name_elem = entity->fparam_list;
    if (name_elem != NULL) {
      curr_node = declaration_base;
      while (name_elem != NULL && curr_node != NULL) {
        if (CATEGORY_OF_OBJ(name_elem->obj) == FORMPARAMOBJECT) {
          param_obj = name_elem->obj->value.objValue;
          if (CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT && VAR_OBJECT(param_obj)) {
            curr_node = pop_node(curr_node->inout_param, param_obj->type_of->match_obj);
          } else if (CATEGORY_OF_OBJ(param_obj) == VALUEPARAMOBJECT ||
              CATEGORY_OF_OBJ(param_obj) == REFPARAMOBJECT) {
            curr_node = pop_node(curr_node->other_param, param_obj->type_of->match_obj);
          } else if (CATEGORY_OF_OBJ(param_obj) == TYPEOBJECT) {
            curr_node = pop_node(curr_node->attr, param_obj->value.typeValue->match_obj);
          } /* if */
        } else {
          curr_node = pop_node(curr_node->symbol, name_elem->obj);
        } /* if */
        name_elem = name_elem->next;
      } /* while */
    } /* if */
#ifdef TRACE_ENTITY
    printf("END pop_entity\n");
#endif
  } /* pop_entity */



void close_entity (progType currentProg)

  {
    entityType entity;
    entityType old_entity;

  /* close_entity */
    entity = currentProg->entity.inactive_list;
    while (entity != NULL) {
      old_entity = entity;
      entity = entity->data.next;
      free_entity(currentProg, old_entity);
    } /* while */
    currentProg->entity.inactive_list = NULL;
  } /* close_entity */



void init_entity (errInfoType *err_info)

  { /* init_entity */
#ifdef TRACE_ENTITY
    printf("BEGIN init_entity\n");
#endif
    prog.entity.inactive_list = NULL;
    if ((prog.entity.literal = new_entity(prog.ident.literal)) == NULL) {
      *err_info = MEMORY_ERROR;
    } /* if */
    if (!ALLOC_RECORD(prog.property.literal, propertyRecord, count.property)) {
      *err_info = MEMORY_ERROR;
    } /* if */
    prog.property.literal->entity = prog.entity.literal;
    prog.property.literal->params = NULL;
    prog.property.literal->file_number = 0;
    prog.property.literal->line = 0;
    prog.property.literal->syNumberInLine = 0;
#ifdef TRACE_ENTITY
    printf("END init_entity\n");
#endif
  } /* init_entity */
