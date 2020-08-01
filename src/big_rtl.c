/********************************************************************/
/*                                                                  */
/*  big_rtl.c     Primitive actions for the bigInteger type.        */
/*  Copyright (C) 1989 - 2006  Thomas Mertes                        */
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
/*  File: seed7/src/big_rtl.c                                       */
/*  Changes: 2005, 2006  Thomas Mertes                              */
/*  Content: Primitive actions for the bigInteger type.             */
/*                                                                  */
/********************************************************************/

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "version.h"
#include "common.h"
#include "heaputl.h"
#include "rtl_err.h"

#undef EXTERN
#define EXTERN
#include "big_rtl.h"



#define LOWER_16(A) ((A) & 0177777L)
#define UPPER_16(A) (((A) >> 16) & 0177777L)
#define LOWER_32(A) ((A) & (uinttype) 037777777777L)

#define BIGDIGIT_MAX  037777777777L

#define BIGDIGIT_MASK 0177777
#define BIGDIGIT_SIGN 0100000

#define IS_NEGATIVE(digit) ((digit) & BIGDIGIT_SIGN)



#ifdef ANSI_C

static void normalize (biginttype big1)
#else

static void normalize (arg1)
biginttype big1;
#endif

  {
    memsizetype pos;
    bigdigittype digit;

  /* normalize */
    pos = big1->size;
    if (pos >= 2) {
      pos--;
      digit = big1->bigdigits[pos];
      if (digit == BIGDIGIT_MASK) {
        do {
          pos--;
          digit = big1->bigdigits[pos];
        } while (pos > 0 && digit == BIGDIGIT_MASK);
        if (!IS_NEGATIVE(digit)) {
          pos++;
        } /* if */
      } else if (digit == 0) {
        do {
          pos--;
          digit = big1->bigdigits[pos];
        } while (pos > 0 && digit == 0);
        if (IS_NEGATIVE(digit)) {
          pos++;
        } /* if */
      } /* if */
      pos++;
      if (big1->size != pos) {
        if (!RESIZE_BIG(big1, big1->size, pos)) {
          raise_error(MEMORY_ERROR);
          return;
        } else {
          COUNT3_BIG(big1->size, pos);
          big1->size = pos;
        } /* if */
      } /* if */
    } /* if */
  } /* normalize */



#ifdef ANSI_C

static biginttype mult_bigdigit (biginttype big1, bigdigittype multiplier,
    doublebigdigittype carry)
#else

static biginttype mult_bigdigit (big1, multiplier, carry)
biginttype big1;
bigdigittype multiplier;
doublebigdigittype carry;
#endif

  {
    memsizetype pos;

  /* mult_bigdigit */
    for (pos = 0; pos < big1->size; pos++) {
      carry += big1->bigdigits[pos] * multiplier;
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
    } /* for */
    if (carry != 0 || IS_NEGATIVE(big1->bigdigits[pos - 1])) {
      if (!RESIZE_BIG(big1, big1->size, big1->size + 1)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        COUNT3_BIG(big1->size, big1->size + 1);
        big1->bigdigits[pos] = carry;
        big1->size++;
      } /* if */
    } /* if */
    return(big1);
  } /* mult_bigdigit */



#ifdef ANSI_C

static biginttype mult10_bigdigit (biginttype big1, doublebigdigittype carry)
#else

static biginttype mult10_bigdigit (big1, carry)
biginttype big1;
doublebigdigittype carry;
#endif

  {
    memsizetype pos;

  /* mult10_bigdigit */
    for (pos = 0; pos < big1->size; pos++) {
      carry += big1->bigdigits[pos] * 10;
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
    } /* for */
    if (carry != 0 || IS_NEGATIVE(big1->bigdigits[pos - 1])) {
      if (!RESIZE_BIG(big1, big1->size, big1->size + 1)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        COUNT3_BIG(big1->size, big1->size + 1);
        big1->bigdigits[pos] = carry;
        big1->size++;
      } /* if */
    } /* if */
    return(big1);
  } /* mult10_bigdigit */



#ifdef ANSI_C

static biginttype add_bigdigit (biginttype big1, bigdigittype digit)
#else

static biginttype add_bigdigit (big1, digit)
biginttype big1;
bigdigittype digit;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry;

  /* add_bigdigit */
    carry = digit;
    for (pos = 0; pos < big1->size; pos++) {
      carry += big1->bigdigits[pos];
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
    } /* for */
    if (IS_NEGATIVE(big1->bigdigits[pos - 1])) {
      if (!RESIZE_BIG(big1, big1->size, big1->size + 1)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        COUNT3_BIG(big1->size, big1->size + 1);
        big1->bigdigits[pos] = (bigdigittype) carry;
        big1->size = pos + 1;
      } /* if */
    } /* if */
    normalize(big1);
    return(big1);
  } /* add_bigdigit */



#ifdef ANSI_C

static bigdigittype div_bigdigit (biginttype big1, bigdigittype digit)
#else

static bigdigittype div_bigdigit (big1, digit)
biginttype big1;
bigdigittype digit;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 0;

  /* div_bigdigit */
    for (pos = big1->size; pos > 0; pos--) {
      carry <<= 8 * sizeof(bigdigittype);
      carry += big1->bigdigits[pos - 1];
      big1->bigdigits[pos - 1] = (bigdigittype) (carry / digit);
      carry %= digit;
    } /* for */
    return(carry);
  } /* div_bigdigit */



#ifdef ANSI_C

static bigdigittype div10_bigdigit (biginttype big1)
#else

static bigdigittype div10_bigdigit (big1)
biginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 0;

  /* div10_bigdigit */
    for (pos = big1->size; pos > 0; pos--) {
      carry <<= 8 * sizeof(bigdigittype);
      carry += big1->bigdigits[pos - 1];
      big1->bigdigits[pos - 1] = (bigdigittype) (carry / 10);
      carry %= 10;
    } /* for */
    return(carry);
  } /* div10_bigdigit */



#ifdef ANSI_C

biginttype negate_big (biginttype big1)
#else

biginttype negate_big (big1)
biginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;
    bigdigittype negative;

  /* negate_big */
      negative = IS_NEGATIVE(big1->bigdigits[big1->size - 1]);
      for (pos = 0; pos < big1->size; pos++) {
        carry += ~big1->bigdigits[pos] & BIGDIGIT_MASK;
        big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
      } /* for */
      if (negative && IS_NEGATIVE(big1->bigdigits[pos - 1])) {
        if (!RESIZE_BIG(big1, pos, pos + 1)) {
          raise_error(MEMORY_ERROR);
          return(NULL);
        } else {
          COUNT3_BIG(pos, pos + 1);
          big1->bigdigits[pos] = 0;
          big1->size++;
        } /* if */
      } /* if */
      return(big1);
  } /* negate_big */



#ifdef ANSI_C

void negate_positive_big (biginttype big1)
#else

void negate_positive_big (big1)
biginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;

  /* negate_positive_big */
      for (pos = 0; pos < big1->size; pos++) {
        carry += ~big1->bigdigits[pos] & BIGDIGIT_MASK;
        big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
      } /* for */
  } /* negate_positive_big */



#ifdef ANSI_C

void positive_copy_of_negative_big (biginttype dest, biginttype big1)
#else

void positive_copy_of_negative_big (big1)
biginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;

  /* positive_copy_of_negative_big */
    for (pos = 0; pos < big1->size; pos++) {
      carry += ~big1->bigdigits[pos] & BIGDIGIT_MASK;
      dest->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
    } /* for */
    if (IS_NEGATIVE(dest->bigdigits[pos - 1])) {
      dest->bigdigits[pos] = 0;
      pos++;
    } /* if */
    dest->size = pos;
  } /* positive_copy_of_negative_big */



#ifdef ANSI_C

void bigDump (biginttype big1)
#else

void bigDump (big1)
biginttype big1;
#endif

  {
    bigdigittype *digitPos;

  /* bigDump */
    printf("[%lu] (%hd",
        big1->size,
        (short int) big1->bigdigits[big1->size - 1]);
    if (big1->size >= 2) {
      for (digitPos = &big1->bigdigits[big1->size - 2]; digitPos >= big1->bigdigits; digitPos--) {
        printf(", %hu", *digitPos);
      } /* for */
    } /* if */
    printf(")  [%04hx",
        big1->bigdigits[big1->size - 1]);
    if (big1->size >= 2) {
      for (digitPos = &big1->bigdigits[big1->size - 2]; digitPos >= big1->bigdigits; digitPos--) {
        printf(", %04hx", *digitPos);
      } /* for */
    } /* if */
    printf("]\n");
  } /* bigDump */



#ifdef ANSI_C

void bigDump2 (biginttype big1)
#else

void bigDump2 (big1)
biginttype big1;
#endif

  {
    bigdigittype *digitPos;

  /* bigDump2 */
    printf("[%lu] [%04hx",
        big1->size,
        big1->bigdigits[big1->size - 1]);
    if (big1->size >= 2) {
      for (digitPos = &big1->bigdigits[big1->size - 2]; digitPos >= big1->bigdigits; digitPos--) {
        printf(", %04hx", *digitPos);
      } /* for */
    } /* if */
    printf("]\n");
  } /* bigDump2 */



#ifdef ANSI_C

biginttype bigAbs (biginttype big1)
#else

biginttype bigAbs (big1)
biginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;
    biginttype result;

  /* bigAbs */
    if (!ALLOC_BIG(result, big1->size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      COUNT_BIG(big1->size);
      result->size = big1->size;
      if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
        for (pos = 0; pos < big1->size; pos++) {
          carry += ~big1->bigdigits[pos] & BIGDIGIT_MASK;
          result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
        } /* for */
        if (IS_NEGATIVE(result->bigdigits[pos - 1])) {
          result->size++;
          if (!RESIZE_BIG(result, big1->size, result->size)) {
            raise_error(MEMORY_ERROR);
            return(NULL);
          } else {
            COUNT3_BIG(big1->size, result->size);
            result->bigdigits[big1->size] = 0;
          } /* if */
        } /* if */
      } else {
        memcpy(result->bigdigits, big1->bigdigits,
            (SIZE_TYPE) big1->size * sizeof(bigdigittype));
      } /* if */
    } /* if */
    return(result);
  } /* bigAbs */



#ifdef ANSI_C

biginttype bigAdd (biginttype big1, biginttype big2)
#else

biginttype bigAdd (big1, big2)
biginttype big1;
biginttype big2;
#endif

  {
    biginttype help_big;
    memsizetype pos;
    doublebigdigittype carry = 0;
    doublebigdigittype big2_sign;
    biginttype result;

  /* bigAdd */
    if (big2->size > big1->size) {
      help_big = big1;
      big1 = big2;
      big2 = help_big;
    } /* if */
    if (!ALLOC_BIG(result, big1->size + 1)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      COUNT_BIG(big1->size + 1);
      for (pos = 0; pos < big2->size; pos++) {
        carry += big1->bigdigits[pos] + big2->bigdigits[pos];
        result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
      } /* for */
      big2_sign = IS_NEGATIVE(big2->bigdigits[pos - 1]) ? BIGDIGIT_MASK : 0;
      for (; pos < big1->size; pos++) {
        carry += big1->bigdigits[pos] + big2_sign;
        result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
      } /* for */
      if (IS_NEGATIVE(big1->bigdigits[pos - 1])) {
        big2_sign--;
      } /* if */
      result->bigdigits[pos] = (bigdigittype) (carry + big2_sign);
      result->size = pos + 1;
      normalize(result);
      return(result);
    } /* if */
  } /* bigAdd */



#ifdef ANSI_C

inttype bigCmp (biginttype big1, biginttype big2)
#else

inttype bigCmp (big1, big2)
biginttype big1;
biginttype big2;
#endif

  {
    bigdigittype big1_negative;
    bigdigittype big2_negative;
    memsizetype pos;

  /* bigCmp */
    big1_negative = IS_NEGATIVE(big1->bigdigits[big1->size - 1]);
    big2_negative = IS_NEGATIVE(big2->bigdigits[big2->size - 1]);
    if (big1_negative != big2_negative) {
      return(big1_negative ? -1 : 1);
    } else if (big1->size != big2->size) {
      return((big1->size < big2->size) != (big1_negative != 0) ? -1 : 1);
    } else {
      pos = big1->size;
      while (pos > 0) {
        pos--;
        if (big1->bigdigits[pos] != big2->bigdigits[pos]) {
          return(big1->bigdigits[pos] < big2->bigdigits[pos] ? -1 : 1);
        } /* if */
      } /* while */
      return(0);
    } /* if */
  } /* bigCmp */



#ifdef ANSI_C

void bigCpy (biginttype *big_to, biginttype big_from)
#else

objecttype bigCpy (big_to, big_from)
biginttype *big_to;
biginttype big_from;
#endif

  {
    memsizetype new_size;
    biginttype big_dest;

  /* bigCpy */
    big_dest = *big_to;
    new_size = big_from->size;
    if (big_dest->size != new_size) {
      if (!ALLOC_BIG(big_dest, new_size)) {
        raise_error(MEMORY_ERROR);
        return;
      } else {
        COUNT_BIG(new_size);
        FREE_BIG(*big_to, (*big_to)->size);
        big_dest->size = new_size;
        *big_to = big_dest;
      } /* if */
    } /* if */
    memcpy(big_dest->bigdigits, big_from->bigdigits,
        (SIZE_TYPE) new_size * sizeof(bigdigittype));
  } /* bigCpy */



#ifdef ANSI_C

void bigDecr (biginttype *big_variable)
#else

void bigDecr (big_variable)
biginttype *big_variable;
#endif

  {
    biginttype big1;
    memsizetype pos;
    doublebigdigittype sum;
    doublebigdigittype carry = BIGDIGIT_MASK;

  /* bigDecr */
    big1 = *big_variable;
    for (pos = 0; carry && pos < big1->size; pos++) {
      sum = carry + big1->bigdigits[pos];
      carry = sum >> 8 * sizeof(bigdigittype);
      big1->bigdigits[pos] = (bigdigittype) (sum & BIGDIGIT_MASK);
    } /* for */
    if (carry) {
      if (!RESIZE_BIG(big1, big1->size, big1->size + 1)) {
        raise_error(MEMORY_ERROR);
      } else {
        COUNT3_BIG(big1->size, big1->size + 1);
        big1->bigdigits[pos] = carry;
        big1->size++;
        *big_variable = big1;
      } /* if */
    } /* if */
  } /* bigDecr */



#ifdef ANSI_C

void bigDestr (biginttype old_bigint)
#else

void bigDestr (old_bigint)
biginttype old_bigint;
#endif

  { /* bigDestr */
    if (old_bigint != NULL) {
      FREE_BIG(old_bigint, old_bigint->size);
    } /* if */
  } /* bigDestr */



#ifdef ANSI_C

booltype bigEq (biginttype big1, biginttype big2)
#else

booltype bigEq (big1, big2)
biginttype big1;
biginttype big2;
#endif

  { /* bigEq */
    if (big1->size == big2->size &&
      memcmp(big1->bigdigits, big2->bigdigits,
          (SIZE_TYPE) big1->size * sizeof(bigdigittype)) == 0) {
      return(TRUE);
    } else {
      return(FALSE);
    } /* if */
  } /* bigEq */



#ifdef OUT_OF_ORDER
#ifdef ANSI_C

void bigIncr (biginttype *big_variable)
#else

void bigIncr (big_variable)
biginttype *big_variable;
#endif

  {
    biginttype big1;
    memsizetype pos;
    doublebigdigittype carry = 1;
    biginttype result;

  /* bigIncr */
    big1 = *big_variable;
    if (!ALLOC_BIG(result, big1->size + 1)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      COUNT_BIG(big1->size + 1);
      for (; pos < big1->size; pos++) {
        carry += big1->bigdigits[pos];
        result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
      } /* for */
      if (IS_NEGATIVE(big1->bigdigits[pos - 1])) {
        result->bigdigits[pos] = (bigdigittype) (carry - 1);
      } else {
        result->bigdigits[pos] = (bigdigittype) carry;
      } /* if */
      result->size = pos + 1;
      normalize(result);
      return(result);
    } /* if */
  } /* bigIncr */
#endif



#ifdef ANSI_C

void bigIncr (biginttype *big_variable)
#else

void bigIncr (big_variable)
biginttype *big_variable;
#endif

  {
    biginttype big1;
    doublebigdigittype sign;
    memsizetype pos;
    doublebigdigittype carry = 1;

  /* bigIncr */
    big1 = *big_variable;
    sign = big1->bigdigits[big1->size - 1] >> 8 * sizeof(bigdigittype) - 1;
    /* printf("  bigIncr size=%ld sign=%ld\n", big1->size, sign); */
    for (pos = 0; /* carry != sign && */ pos < big1->size; pos++) {
      carry += big1->bigdigits[pos];
      big1->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
      carry >>= 8 * sizeof(bigdigittype);
      /* printf("  carry=%ld\n", carry); */
    } /* for */
    /* printf("sign=%d, high_bit=%d\n", sign, big1->bigdigits[big1->size - 1] >> 8 * sizeof(bigdigittype) - 1); */
    if (!sign && big1->bigdigits[big1->size - 1] >> 8 * sizeof(bigdigittype) - 1) {
      /* printf(" **** A carry=%ld\n", carry); */
      if (!RESIZE_BIG(big1, big1->size, big1->size + 1)) {
        raise_error(MEMORY_ERROR);
      } else {
        COUNT3_BIG(big1->size, big1->size + 1);
        big1->bigdigits[pos] = carry;
        big1->size++;
        *big_variable = big1;
      } /* if */
    } else if (big1->size >= 2 &&
        big1->bigdigits[big1->size - 1] == BIGDIGIT_MASK &&
        big1->bigdigits[big1->size - 2] >> 8 * sizeof(bigdigittype) - 1) {
      /* printf(" **** B carry=%ld\n", carry); */
      if (!RESIZE_BIG(big1, big1->size, big1->size - 1)) {
        raise_error(MEMORY_ERROR);
      } else {
        COUNT3_BIG(big1->size, big1->size - 1);
        big1->size--;
        *big_variable = big1;
      } /* if */
    } /* if */
  } /* bigIncr */



#ifdef ANSI_C

biginttype bigMinus (biginttype big1)
#else

biginttype bigMinus (big1)
biginttype big1;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;
    bigdigittype negative;
    biginttype result;

  /* bigMinus */
    if (!ALLOC_BIG(result, big1->size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      COUNT_BIG(big1->size);
      result->size = big1->size;
      negative = IS_NEGATIVE(big1->bigdigits[big1->size - 1]);
      for (pos = 0; pos < big1->size; pos++) {
        carry += ~big1->bigdigits[pos] & BIGDIGIT_MASK;
        result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
      } /* for */
      if (negative && IS_NEGATIVE(result->bigdigits[pos - 1])) {
        if (!RESIZE_BIG(result, result->size, result->size + 1)) {
          raise_error(MEMORY_ERROR);
          return(NULL);
        } else {
          COUNT3_BIG(result->size, result->size + 1);
          result->size++;
          result->bigdigits[big1->size] = 0;
        } /* if */
      } /* if */
      return(result);
    } /* if */
  } /* bigMinus */



#ifdef ANSI_C

biginttype bigMult (biginttype big1, biginttype big2)
#else

biginttype bigMult (big1, big2)
biginttype big1;
biginttype big2;
#endif

  {
    memsizetype pos1;
    memsizetype pos2;
    doublebigdigittype carry = 0;
    biginttype result;

  /* bigMult */
    if (!ALLOC_BIG(result, big1->size + big2->size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      COUNT_BIG(big1->size + big2->size);
      for (pos2 = 0; pos2 < big2->size; pos2++) {
        carry += big1->bigdigits[0] * big2->bigdigits[pos2];
        result->bigdigits[pos2] = (bigdigittype) (carry & BIGDIGIT_MASK);
        carry >>= 8 * sizeof(bigdigittype);
      } /* for */
      result->bigdigits[big2->size] = (bigdigittype) (carry & BIGDIGIT_MASK);
      for (pos1 = 1; pos1 < big1->size; pos1++) {
        carry = 0;
        for (pos2 = 0; pos2 < big2->size; pos2++) {
          carry += big1->bigdigits[pos1] * big2->bigdigits[pos1];
          result->bigdigits[pos1 + pos2] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
        } /* for */
        result->bigdigits[pos1 + big2->size] = (bigdigittype) (carry & BIGDIGIT_MASK);
      } /* for */
      result->size = big1->size + big2->size;
      normalize(result);
      return(result);
    } /* if */
  } /* bigMult */



#ifdef ANSI_C

booltype bigNe (biginttype big1, biginttype big2)
#else

booltype bigNe (big1, big2)
biginttype big1;
biginttype big2;
#endif

  { /* bigNe */
    if (big1->size != big2->size ||
      memcmp(big1->bigdigits, big2->bigdigits,
          (SIZE_TYPE) big1->size * sizeof(bigdigittype)) != 0) {
      return(TRUE);
    } else {
      return(FALSE);
    } /* if */
  } /* bigNe */



#ifdef ANSI_C

biginttype bigParse (stritype stri)
#else

biginttype bigParse (stri)
stritype stri;
#endif

  {
    booltype okay;
    booltype negative;
    memsizetype position;
    doublebigdigittype digitval;
    biginttype integer_value;

  /* bigParse */
    if (!ALLOC_BIG(integer_value, 1)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      COUNT_BIG(1);
      integer_value->size = 1;
      integer_value->bigdigits[0] = 0;
      okay = TRUE;
      position = 0;
      if (stri->size >= 1 && stri->mem[0] == ((strelemtype) '-')) {
        negative = TRUE;
        position++;
      } else {
        negative = FALSE;
      } /* if */
      while (position < stri->size &&
          stri->mem[position] >= ((strelemtype) '0') &&
          stri->mem[position] <= ((strelemtype) '9')) {
        digitval = ((doublebigdigittype) stri->mem[position]) - ((bigdigittype) '0');
        integer_value = mult10_bigdigit(integer_value, digitval);
        position++;
      } /* while */
      if (position == 0 || position < stri->size) {
        okay = FALSE;
      } /* if */
      if (okay) {
        if (negative) {
          negate_positive_big(integer_value);
        } /* if */
        return(integer_value);
      } else {
        raise_error(RANGE_ERROR);
        return(NULL);
      } /* if */
    } /* if */
  } /* bigParse */



#ifdef ANSI_C

biginttype bigSbtr (biginttype big1, biginttype big2)
#else

biginttype bigSbtr (big1, big2)
biginttype big1;
biginttype big2;
#endif

  {
    memsizetype pos;
    doublebigdigittype carry = 1;
    doublebigdigittype big1_sign;
    doublebigdigittype big2_sign;
    biginttype result;

  /* bigSbtr */
    if (big1->size >= big2->size) {
      if (!ALLOC_BIG(result, big1->size + 1)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        COUNT_BIG(big1->size + 1);
        for (pos = 0; pos < big2->size; pos++) {
          carry += big1->bigdigits[pos] + ~big2->bigdigits[pos];
          result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
        } /* for */
        big2_sign = IS_NEGATIVE(big2->bigdigits[pos - 1]) ? 0: BIGDIGIT_MASK;
        for (; pos < big1->size; pos++) {
          carry += big1->bigdigits[pos] + big2_sign;
          result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
        } /* for */
        if (IS_NEGATIVE(big1->bigdigits[pos - 1])) {
          big2_sign--;
        } /* if */
        result->bigdigits[pos] = (bigdigittype) (carry + big2_sign);
        result->size = pos + 1;
        normalize(result);
        return(result);
      } /* if */
    } else {
      if (!ALLOC_BIG(result, big2->size + 1)) {
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        COUNT_BIG(big2->size + 1);
        for (pos = 0; pos < big1->size; pos++) {
          carry += big1->bigdigits[pos] + ~big2->bigdigits[pos];
          result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
        } /* for */
        big1_sign = IS_NEGATIVE(big1->bigdigits[pos - 1]) ? BIGDIGIT_MASK : 0;
        for (; pos < big2->size; pos++) {
          carry += big1_sign + ~big2->bigdigits[pos];
          result->bigdigits[pos] = (bigdigittype) (carry & BIGDIGIT_MASK);
          carry >>= 8 * sizeof(bigdigittype);
        } /* for */
        if (IS_NEGATIVE(big2->bigdigits[pos - 1])) {
          big1_sign--;
        } /* if */
        result->bigdigits[pos] = (bigdigittype) (carry + big1_sign);
        result->size = pos + 1;
        normalize(result);
        return(result);
      } /* if */
    } /* if */
  } /* bigSbtr */



#ifdef ANSI_C

stritype bigStr (biginttype big1)
#else

stritype bigStr (big1)
biginttype big1;
#endif

  {
    biginttype help_big;
    memsizetype pos;
    strelemtype digit_ch;
    memsizetype result_size;
    stritype result;

  /* bigStr */
    result_size = 256;
    if (!ALLOC_STRI(result, result_size)) {
      raise_error(MEMORY_ERROR);
      return(NULL);
    } else {
      COUNT_STRI(result_size);
      if (!ALLOC_BIG(help_big, big1->size + 1)) {
        FREE_STRI(result, result_size);
        raise_error(MEMORY_ERROR);
        return(NULL);
      } else {
        COUNT_BIG(big1->size + 1);
        pos = 0;
        if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
          positive_copy_of_negative_big(help_big, big1);
          result->mem[pos] = '-';
          pos++;
        } else {
          help_big->size = big1->size;
          memcpy(help_big->bigdigits, big1->bigdigits,
              (SIZE_TYPE) big1->size * sizeof(bigdigittype));
        } /* if */
        do {
          if (pos >= result_size) {
            if (!RESIZE_STRI(result, result_size, result_size + 256)) {
              FREE_STRI(result, result_size);
              FREE_BIG(help_big, big1->size + 1);
              raise_error(MEMORY_ERROR);
              return(NULL);
            } else {
              COUNT3_STRI(result_size, result_size + 256);
              result_size += 256;
            } /* if */
          } /* if */
          result->mem[pos] = '0' + div10_bigdigit(help_big);
          pos++;
          if (help_big->bigdigits[help_big->size - 1] == 0) {
            help_big->size--;
          } /* if */
          /* normalize(help_big); */
        } while (help_big->size > 1 || help_big->bigdigits[0] != 0);
        FREE_BIG(help_big, big1->size + 1);
        result->size = pos;
        if (!RESIZE_STRI(result, result_size, pos)) {
          FREE_STRI(result, result_size);
          raise_error(MEMORY_ERROR);
          return(NULL);
        } else {
          COUNT3_STRI(result_size, pos);
          if (IS_NEGATIVE(big1->bigdigits[big1->size - 1])) {
            for (pos = 1; pos <= result->size >> 1; pos++) {
              digit_ch = result->mem[pos];
              result->mem[pos] = result->mem[result->size - pos];
              result->mem[result->size - pos] = digit_ch;
            } /* for */
          } else {
            for (pos = 0; pos < result->size >> 1; pos++) {
              digit_ch = result->mem[pos];
              result->mem[pos] = result->mem[result->size - pos - 1];
              result->mem[result->size - pos - 1] = digit_ch;
            } /* for */
          } /* if */
          return(result);
        } /* if */
      } /* if */
    } /* if */
  } /* bigStr */
