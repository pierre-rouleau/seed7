/********************************************************************/
/*                                                                  */
/*  s7   Seed7 interpreter                                          */
/*  Copyright (C) 1990 - 2014  Thomas Mertes                        */
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
/*  Module: Library                                                 */
/*  File: seed7/src/intlib.c                                        */
/*  Changes: 1992 - 1994, 2000, 2005, 2013, 2014  Thomas Mertes     */
/*  Content: All primitive actions for the integer type.            */
/*                                                                  */
/********************************************************************/

#include "version.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "common.h"
#include "data.h"
#include "heaputl.h"
#include "flistutl.h"
#include "syvarutl.h"
#include "objutl.h"
#include "runerr.h"
#include "int_rtl.h"

#undef EXTERN
#define EXTERN
#include "intlib.h"


static intType fact[] = {
    1, 1, 2, 6, 24, 120, 720, 5040, 40320,
    362880, 3628800, 39916800, 479001600,
#if INTTYPE_SIZE == 64
    INT_SUFFIX(6227020800), INT_SUFFIX(87178291200),
    INT_SUFFIX(1307674368000), INT_SUFFIX(20922789888000),
    INT_SUFFIX(355687428096000), INT_SUFFIX(6402373705728000),
    INT_SUFFIX(121645100408832000), INT_SUFFIX(2432902008176640000)
#endif
  };



/**
 *  Generic Destr function.
 *  Used for all types that require no special action to
 *  destroy the data.
 */
objectType gen_destr (listType arguments)

  { /* gen_destr */
    SET_UNUSED_FLAG(arg_1(arguments));
    return SYS_EMPTY_OBJECT;
  } /* gen_destr */



/**
 *  Compute the absolute value of an integer number.
 *  @return the absolute value.
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_abs (listType arguments)

  {
    intType number;

  /* int_abs */
    isit_int(arg_1(arguments));
    number = take_int(arg_1(arguments));
    if (number < 0) {
#if defined CHECK_INT_OVERFLOW && defined TWOS_COMPLEMENT_INTTYPE
      if (unlikely(number == INTTYPE_MIN)) {
        /* Changing the sign of the most negative number in twos */
        /* complement arithmetic triggers an overflow.           */
        return raise_exception(SYS_OVF_EXCEPTION);
      } /* if */
#endif
      number = -number;
    } /* if */
    return bld_int_temp(number);
  } /* int_abs */



/**
 *  Add two integer numbers.
 *  @return the sum of the two numbers.
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_add (listType arguments)

  {
    intType summand1;
    intType summand2;

  /* int_add */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    summand1 = take_int(arg_1(arguments));
    summand2 = take_int(arg_3(arguments));
#ifdef CHECK_INT_OVERFLOW
    if (summand2 < 0) {
      if (unlikely(summand1 < INTTYPE_MIN - summand2)) {
        return raise_exception(SYS_OVF_EXCEPTION);
      } /* if */
    } else {
      if (unlikely(summand1 > INTTYPE_MAX - summand2)) {
        return raise_exception(SYS_OVF_EXCEPTION);
      } /* if */
    } /* if */
#endif
    return bld_int_temp(summand1 + summand2);
  } /* int_add */



/**
 *  Compute a bitwise 'and' of two integer values.
 *  @return the bitwise 'and' of the two values.
 */
objectType int_and (listType arguments)

  { /* int_and */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    return bld_int_temp(
        take_int(arg_1(arguments)) & take_int(arg_3(arguments)));
  } /* int_and */



/**
 *  Compute a bitwise 'and' and assign the result back.
 */
objectType int_and_assign (listType arguments)

  {
    objectType int_variable;

  /* int_and_assign */
    int_variable = arg_1(arguments);
    isit_int(int_variable);
    is_variable(int_variable);
    isit_int(arg_3(arguments));
    int_variable->value.intValue &= take_int(arg_3(arguments));
    return SYS_EMPTY_OBJECT;
  } /* int_and_assign */



/**
 *  Binomial coefficient
 *  @return n over k
 */
objectType int_binom (listType arguments)

  { /* int_binom */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    return bld_int_temp(
        intBinom(take_int(arg_1(arguments)), take_int(arg_3(arguments))));
  } /* int_binom */



/**
 *  Number of bits in the minimal two's-complement representation.
 *  The high bits equivalent to the sign bit are not part of the
 *  minimal two's-complement representation.
 *  @return the number of bits.
 */
objectType int_bit_length (listType arguments)

  { /* int_bit_length */
    isit_int(arg_1(arguments));
    return bld_int_temp(
        intBitLength(take_int(arg_1(arguments))));
  } /* int_bit_length */



/**
 *  Convert an integer into a big-endian string of bytes.
 *  The result uses binary representation with a base of 256.
 *  The result contains chars (bytes) with an ordinal <= 255.
 *  @param number/arg_1 Integer number to be converted.
 *  @param isSigned/arg_2 Determines the signedness of the result.
 *         When 'isSigned' is TRUE the result is encoded with the
 *         twos-complement representation. In this case a negative
 *         'number' is converted to a result where the most significant
 *         byte has an ordinal >= 128.
 *  @return a string with the shortest binary representation of 'number'.
 *  @exception RANGE_ERROR When 'number' is negative and 'isSigned' is FALSE.
 *  @exception MEMORY_ERROR Not enough memory to represent the result.
 */
objectType int_bytesBe (listType arguments)

  { /* int_bytesBe */
    isit_int(arg_1(arguments));
    isit_bool(arg_2(arguments));
    return bld_stri_temp(
        intBytesBe(take_int(arg_1(arguments)),
                   take_bool(arg_2(arguments)) == SYS_TRUE_OBJECT));
  } /* int_bytesBe */



/**
 *  Convert a string of bytes (interpreted as big-endian) to an integer.
 *  @param byteStri/arg_1 String of bytes to be converted. The bytes
 *         are interpreted as binary big-endian representation with a
 *         base of 256.
 *  @param isSigned/arg_2 Determines the signedness of 'byteStri'.
 *         When 'isSigned' is TRUE 'byteStri' is interpreted as
 *         signed value in the twos-complement representation.
 *         In this case the result is negative when the most significant
 *         byte (the first byte) has an ordinal >= 128.
 *  @return an integer created from 'byteStri'.
 *  @exception RANGE_ERROR When characters beyond '\255;' are present or
 *             when the result value cannot be represented with an integer.
 */
objectType int_bytesBe2Int (listType arguments)

  { /* int_bytesBe2Int */
    isit_stri(arg_1(arguments));
    isit_bool(arg_2(arguments));
    return bld_int_temp(
        intBytesBe2Int(take_stri(arg_1(arguments)),
                       take_bool(arg_2(arguments)) == SYS_TRUE_OBJECT));
  } /* int_bytesBe2Int */



/**
 *  Convert an integer into a little-endian string of bytes.
 *  The result uses binary representation with a base of 256.
 *  The result contains chars (bytes) with an ordinal <= 255.
 *  @param number/arg_1 Integer number to be converted.
 *  @param isSigned/arg_2 Determines the signedness of the result.
 *         When 'isSigned' is TRUE the result is encoded with the
 *         twos-complement representation. In this case a negative
 *         'number' is converted to a result where the most significant
 *         byte has an ordinal >= 128.
 *  @return a string with the shortest binary representation of 'number'.
 *  @exception RANGE_ERROR When 'number' is negative and 'isSigned' is FALSE.
 *  @exception MEMORY_ERROR Not enough memory to represent the result.
 */
objectType int_bytesLe (listType arguments)

  { /* int_bytesLe */
    isit_int(arg_1(arguments));
    isit_bool(arg_2(arguments));
    return bld_stri_temp(
        intBytesLe(take_int(arg_1(arguments)),
                   take_bool(arg_2(arguments)) == SYS_TRUE_OBJECT));
  } /* int_bytesLe */



/**
 *  Convert a string of bytes (interpreted as little-endian) to an integer.
 *  @param byteStri/arg_1 String of bytes to be converted. The bytes
 *         are interpreted as binary little-endian representation with a
 *         base of 256.
 *  @param isSigned/arg_2 Determines the signedness of 'byteStri'.
 *         When 'isSigned' is TRUE 'byteStri' is interpreted as
 *         signed value in the twos-complement representation.
 *         In this case the result is negative when the most significant
 *         byte (the last byte) has an ordinal >= 128.
 *  @return an integer created from 'byteStri'.
 *  @exception RANGE_ERROR When characters beyond '\255;' are present or
 *             when the result value cannot be represented with an integer.
 */
objectType int_bytesLe2Int (listType arguments)

  { /* int_bytesLe2Int */
    isit_stri(arg_1(arguments));
    isit_bool(arg_2(arguments));
    return bld_int_temp(
        intBytesLe2Int(take_stri(arg_1(arguments)),
                       take_bool(arg_2(arguments)) == SYS_TRUE_OBJECT));
  } /* int_bytesLe2Int */



/**
 *  Compare two integer numbers.
 *  @return -1, 0 or 1 if the first argument is considered to be
 *          respectively less than, equal to, or greater than the
 *          second.
 */
objectType int_cmp (listType arguments)

  {
    intType number1;
    intType number2;
    intType signumValue;

  /* int_cmp */
    isit_int(arg_1(arguments));
    isit_int(arg_2(arguments));
    number1 = take_int(arg_1(arguments));
    number2 = take_int(arg_2(arguments));
    if (number1 < number2) {
      signumValue = -1;
    } else if (number1 > number2) {
      signumValue = 1;
    } else {
      signumValue = 0;
    } /* if */
    return bld_int_temp(signumValue);
  } /* int_cmp */



/**
 *  Convert to integer.
 *  @return the unchanged number.
 */
objectType int_conv (listType arguments)

  { /* int_conv */
    isit_int(arg_3(arguments));
    return bld_int_temp(take_int(arg_3(arguments)));
  } /* int_conv */



objectType int_cpy (listType arguments)

  {
    objectType int_variable;

  /* int_cpy */
    int_variable = arg_1(arguments);
    isit_int(int_variable);
    is_variable(int_variable);
    isit_int(arg_3(arguments));
    int_variable->value.intValue = take_int(arg_3(arguments));
    return SYS_EMPTY_OBJECT;
  } /* int_cpy */



objectType int_create (listType arguments)

  { /* int_create */
    isit_int(arg_3(arguments));
    SET_CATEGORY_OF_OBJ(arg_1(arguments), INTOBJECT);
    arg_1(arguments)->value.intValue = take_int(arg_3(arguments));
    return SYS_EMPTY_OBJECT;
  } /* int_create */



/**
 *  Decrement an integer variable.
 *  Decrements the number by 1.
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_decr (listType arguments)

  {
    objectType int_variable;

  /* int_decr */
    int_variable = arg_1(arguments);
    isit_int(int_variable);
    is_variable(int_variable);
#ifdef CHECK_INT_OVERFLOW
    if (unlikely(take_int(int_variable) == INTTYPE_MIN)) {
      return raise_exception(SYS_OVF_EXCEPTION);
    } /* if */
#endif
    int_variable->value.intValue--;
    return SYS_EMPTY_OBJECT;
  } /* int_decr */



/**
 *  Integer division truncated towards zero.
 *  The remainder of this division is computed with int_rem.
 *  This function assumes that the C operator / truncates
 *  towards zero. Tests in chkint.sd7 find out, if this
 *  assumption is wrong.
 *  @return the quotient of the integer division.
 *  @exception NUMERIC_ERROR When a division by zero occurs
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_div (listType arguments)

  {
    intType dividend;
    intType divisor;

  /* int_div */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    dividend = take_int(arg_1(arguments));
    divisor = take_int(arg_3(arguments));
    if (unlikely(divisor == 0)) {
      return raise_exception(SYS_NUM_EXCEPTION);
#if defined CHECK_INT_OVERFLOW && defined TWOS_COMPLEMENT_INTTYPE
    } else if (unlikely(divisor == -1 && dividend == INTTYPE_MIN)) {
      /* A division of the most negative number by -1 is equivalent */
      /* to changing the sign of the most negative number. In twos  */
      /* complement arithmetic this triggers an overflow.           */
      return raise_exception(SYS_OVF_EXCEPTION);
#endif
    } /* if */
    return bld_int_temp(dividend / divisor);
  } /* int_div */



/**
 *  Check if two integer numbers are equal.
 *  @return TRUE if the two numbers are equal,
 *          FALSE otherwise.
 */
objectType int_eq (listType arguments)

  { /* int_eq */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    if (take_int(arg_1(arguments)) ==
        take_int(arg_3(arguments))) {
      return SYS_TRUE_OBJECT;
    } else {
      return SYS_FALSE_OBJECT;
    } /* if */
  } /* int_eq */



/**
 *  Compute the factorial of a number.
 *  @return the factorial of the number.
 *  @exception NUMERIC_ERROR The number is negative or the result
 *             does not fit into an integer.
 */
objectType int_fact (listType arguments)

  {
    int number;

  /* int_fact */
    isit_int(arg_2(arguments));
    number = (int) take_int(arg_2(arguments));
    if (number < 0 || (size_t) number >= sizeof(fact) / sizeof(intType)) {
      return raise_exception(SYS_NUM_EXCEPTION);
    } /* if */
    return bld_int_temp(fact[number]);
  } /* int_fact */



/**
 *  Check if number1 is greater than or equal to number2.
 *  @return TRUE if number1 is greater than or equal to number2,
 *          FALSE otherwise.
 */
objectType int_ge (listType arguments)

  { /* int_ge */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    if (take_int(arg_1(arguments)) >=
        take_int(arg_3(arguments))) {
      return SYS_TRUE_OBJECT;
    } else {
      return SYS_FALSE_OBJECT;
    } /* if */
  } /* int_ge */



/**
 *  Increment an integer variable by a delta.
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_grow (listType arguments)

  {
    objectType int_variable;
    intType delta;

  /* int_grow */
    int_variable = arg_1(arguments);
    isit_int(int_variable);
    is_variable(int_variable);
    isit_int(arg_3(arguments));
    delta = take_int(arg_3(arguments));
#ifdef CHECK_INT_OVERFLOW
    {
      intType number;

      number = take_int(int_variable);
      if (delta < 0) {
        if (unlikely(number < INTTYPE_MIN - delta)) {
          return raise_exception(SYS_OVF_EXCEPTION);
        } /* if */
      } else {
        if (unlikely(number > INTTYPE_MAX - delta)) {
          return raise_exception(SYS_OVF_EXCEPTION);
        } /* if */
      } /* if */
    }
#endif
    int_variable->value.intValue += delta;
    return SYS_EMPTY_OBJECT;
  } /* int_grow */



/**
 *  Check if number1 is greater than number2.
 *  @return TRUE if number1 is greater than number2,
 *          FALSE otherwise.
 */
objectType int_gt (listType arguments)

  { /* int_gt */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    if (take_int(arg_1(arguments)) >
        take_int(arg_3(arguments))) {
      return SYS_TRUE_OBJECT;
    } else {
      return SYS_FALSE_OBJECT;
    } /* if */
  } /* int_gt */



/**
 *  Compute the hash value of an integer number.
 *  @return the hash value.
 */
objectType int_hashcode (listType arguments)

  { /* int_hashcode */
    isit_int(arg_1(arguments));
    return bld_int_temp(take_int(arg_1(arguments)));
  } /* int_hashcode */



/**
 *  Increment an integer variable.
 *  Increments the number by 1.
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_incr (listType arguments)

  {
    objectType int_variable;

  /* int_incr */
    int_variable = arg_1(arguments);
    isit_int(int_variable);
    is_variable(int_variable);
#ifdef CHECK_INT_OVERFLOW
    if (unlikely(take_int(int_variable) == INTTYPE_MAX)) {
      return raise_exception(SYS_OVF_EXCEPTION);
    } /* if */
#endif
    int_variable->value.intValue++;
    return SYS_EMPTY_OBJECT;
  } /* int_incr */



/**
 *  Check if number1 is less than or equal to number2.
 *  @return TRUE if number1 is less than or equal to number2,
 *          FALSE otherwise.
 */
objectType int_le (listType arguments)

  { /* int_le */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    if (take_int(arg_1(arguments)) <=
        take_int(arg_3(arguments))) {
      return SYS_TRUE_OBJECT;
    } else {
      return SYS_FALSE_OBJECT;
    } /* if */
  } /* int_le */



/**
 *  Compute the truncated base 10 logarithm of an integer number.
 *  The definition of log10 is extended by defining log10(0) = -1.
 *  @return the truncated base 10 logarithm.
 *  @exception NUMERIC_ERROR The number is negative.
 */
objectType int_log10 (listType arguments)

  { /* int_log10 */
    isit_int(arg_1(arguments));
    return bld_int_temp(
        intLog10(take_int(arg_1(arguments))));
  } /* int_log10 */



/**
 *  Compute the truncated base 2 logarithm of an integer number.
 *  The definition of log2 is extended by defining log2(0) = -1.
 *  @return the truncated base 2 logarithm.
 *  @exception NUMERIC_ERROR The number is negative.
 */
objectType int_log2 (listType arguments)

  { /* int_log2 */
    isit_int(arg_1(arguments));
    return bld_int_temp(
        intLog2(take_int(arg_1(arguments))));
  } /* int_log2 */



/**
 *  Index of the lowest-order one bit.
 *  For A <> 0 this is equal to the number of lowest-order zero bits.
 *  @return the number of lowest-order zero bits or -1 for lowestSetBit(0).
 */
objectType int_lowest_set_bit (listType arguments)

  { /* int_lowest_set_bit */
    isit_int(arg_1(arguments));
    return bld_int_temp(
        intLowestSetBit(take_int(arg_1(arguments))));
  } /* int_lowest_set_bit */



objectType int_lpad0 (listType arguments)

  { /* int_lpad0 */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    return bld_stri_temp(
        intLpad0(take_int(arg_1(arguments)), take_int(arg_3(arguments))));
  } /* int_lpad0 */



/**
 *  Shift an integer number left by lshift bits.
 *  A << B is equivalent to A * 2_ ** B
 *  @return the left shifted number.
 *  @exception OVERFLOW_ERROR When the shift amount is
 *             negative or greater equal INTTYPE_SIZE or
 *             when an integer overflow occurs.
 */
objectType int_lshift (listType arguments)

  {
    intType number;
    intType lshift;

  /* int_lshift */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    number = take_int(arg_1(arguments));
    lshift = take_int(arg_3(arguments));
#ifdef CHECK_INT_OVERFLOW
    if (unlikely(lshift < 0 || lshift >= INTTYPE_SIZE)) {
      return raise_exception(SYS_OVF_EXCEPTION);
    } /* if */
    if (number < 0) {
#ifdef RSHIFT_DOES_SIGN_EXTEND
      if (unlikely(number < INTTYPE_MIN >> lshift)) {
#else
      if (unlikely(number < ~(~INTTYPE_MIN >> lshift))) {
#endif
        return raise_exception(SYS_OVF_EXCEPTION);
      } /* if */
    } else {
      if (unlikely(number > INTTYPE_MAX >> lshift)) {
        return raise_exception(SYS_OVF_EXCEPTION);
      } /* if */
    } /* if */
#endif
    return bld_int_temp(number << lshift);
  } /* int_lshift */



/**
 *  Shift a number left by lshift bits and assign the result back to number.
 *  @exception OVERFLOW_ERROR When the shift amount is
 *             negative or greater equal INTTYPE_SIZE or
 *             when an integer overflow occurs.
 */
objectType int_lshift_assign (listType arguments)

  {
    objectType int_variable;
    intType lshift;

  /* int_lshift_assign */
    int_variable = arg_1(arguments);
    isit_int(int_variable);
    is_variable(int_variable);
    isit_int(arg_3(arguments));
    lshift = take_int(arg_3(arguments));
#ifdef CHECK_INT_OVERFLOW
    if (unlikely(lshift < 0 || lshift >= INTTYPE_SIZE)) {
      return raise_exception(SYS_OVF_EXCEPTION);
    } /* if */
    {
      intType number;

      number = take_int(int_variable);
      if (number < 0) {
#ifdef RSHIFT_DOES_SIGN_EXTEND
        if (unlikely(number < INTTYPE_MIN >> lshift)) {
#else
        if (unlikely(number < ~(~INTTYPE_MIN >> lshift))) {
#endif
          return raise_exception(SYS_OVF_EXCEPTION);
        } /* if */
      } else {
        if (unlikely(number > INTTYPE_MAX >> lshift)) {
          return raise_exception(SYS_OVF_EXCEPTION);
        } /* if */
      } /* if */
    }
#endif
    int_variable->value.intValue <<= lshift;
    return SYS_EMPTY_OBJECT;
  } /* int_lshift_assign */



/**
 *  Check if number1 is less than number2.
 *  @return TRUE if number1 is less than number2,
 *          FALSE otherwise.
 */
objectType int_lt (listType arguments)

  { /* int_lt */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    if (take_int(arg_1(arguments)) <
        take_int(arg_3(arguments))) {
      return SYS_TRUE_OBJECT;
    } else {
      return SYS_FALSE_OBJECT;
    } /* if */
  } /* int_lt */



/**
 *  Integer division truncated towards negative infinity.
 *  The modulo (remainder) of this division is computed with int_mod.
 *  Therefore this division is called modulo division (mdiv).
 *  @return the quotient of the integer division.
 *  @exception NUMERIC_ERROR When a division by zero occurs.
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_mdiv (listType arguments)

  {
    intType dividend;
    intType divisor;
    intType quotient;

  /* int_mdiv */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    dividend = take_int(arg_1(arguments));
    divisor = take_int(arg_3(arguments));
    if (unlikely(divisor == 0)) {
      return raise_exception(SYS_NUM_EXCEPTION);
#if defined CHECK_INT_OVERFLOW && defined TWOS_COMPLEMENT_INTTYPE
    } else if (unlikely(divisor == -1 && dividend == INTTYPE_MIN)) {
      /* A division of the most negative number by -1 is equivalent */
      /* to changing the sign of the most negative number. In twos  */
      /* complement arithmetic this triggers an overflow.           */
      return raise_exception(SYS_OVF_EXCEPTION);
#endif
    } /* if */
    if (dividend > 0 && divisor < 0) {
      quotient = (dividend - 1) / divisor - 1;
    } else if (dividend < 0 && divisor > 0) {
      quotient = (dividend + 1) / divisor - 1;
    } else {
      quotient = dividend / divisor;
    } /* if */
    return bld_int_temp(quotient);
  } /* int_mdiv */



/**
 *  Compute the modulo (remainder) of the integer division int_mdiv.
 *  The modulo has the same sign as the divisor.
 *  @return the modulo of the integer division.
 *  @exception NUMERIC_ERROR When a division by zero occurs.
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_mod (listType arguments)

  {
    intType dividend;
    intType divisor;
    intType modulo;

  /* int_mod */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    dividend = take_int(arg_1(arguments));
    divisor = take_int(arg_3(arguments));
    if (unlikely(divisor == 0)) {
      return raise_exception(SYS_NUM_EXCEPTION);
#if defined CHECK_INT_OVERFLOW && defined TWOS_COMPLEMENT_INTTYPE
    } else if (unlikely(divisor == -1 && dividend == INTTYPE_MIN)) {
      /* To compute the modulo the processor does a division        */
      /* by -1. A division by -1 is equivalent to changing the      */
      /* sign. For the most negative number changing the sign       */
      /* triggers an overflow.                                      */
      return raise_exception(SYS_OVF_EXCEPTION);
#endif
    } else {
      modulo = dividend % divisor;
      if ((dividend < 0) ^ (divisor < 0) && modulo != 0) {
        modulo = modulo + divisor;
      } /* if */
    } /* if */
    return bld_int_temp(modulo);
  } /* int_mod */



/**
 *  Multiply two integer numbers.
 *  @return the product of the two numbers.
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_mult (listType arguments)

  {
    intType factor1;
    intType factor2;
    intType product;

  /* int_mult */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    factor1 = take_int(arg_1(arguments));
    factor2 = take_int(arg_3(arguments));
#ifdef CHECK_INT_OVERFLOW
#ifdef HAS_DOUBLE_INTTYPE
    {
      doubleIntType doubleProduct = (doubleIntType) factor1 * (doubleIntType) factor2;
      product = (intType) doubleProduct;
      if ((doubleIntType) product != doubleProduct) {
        return raise_exception(SYS_OVF_EXCEPTION);
      } /* if */
    }
#else
    if (inHalfIntTypeRange(factor1) && inHalfIntTypeRange(factor2)) {
      product = factor1 * factor2;
    } else {
      product = intSafeMult(factor1, factor2);
    } /* if */
#endif
#else
    product = factor1 * factor2;
#endif
    return bld_int_temp(product);
  } /* int_mult */



/**
 *  Multiply an integer number by a factor and assign the result back to number.
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_mult_assign (listType arguments)

  {
    objectType int_variable;
    intType factor;

  /* int_mult_assign */
    int_variable = arg_1(arguments);
    isit_int(int_variable);
    is_variable(int_variable);
    isit_int(arg_3(arguments));
    factor = take_int(arg_3(arguments));
#ifdef CHECK_INT_OVERFLOW
    {
      intType number;

      number = take_int(int_variable);
      if (inHalfIntTypeRange(number) && inHalfIntTypeRange(factor)) {
        int_variable->value.intValue *= factor;
      } else {
        int_variable->value.intValue = intSafeMult(number, factor);
      } /* if */
    }
#else
    int_variable->value.intValue *= factor;
#endif
    return SYS_EMPTY_OBJECT;
  } /* int_mult_assign */



/**
 *  Check if two integer numbers are not equal.
 *  @return FALSE if both numbers are equal,
 *          TRUE otherwise.
 */
objectType int_ne (listType arguments)

  { /* int_ne */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    if (take_int(arg_1(arguments)) !=
        take_int(arg_3(arguments))) {
      return SYS_TRUE_OBJECT;
    } else {
      return SYS_FALSE_OBJECT;
    } /* if */
  } /* int_ne */



/**
 *  Minus sign, negate an integer number.
 *  @return the negated value of the number.
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_negate (listType arguments)

  {
    intType number;

  /* int_negate */
    isit_int(arg_2(arguments));
    number = take_int(arg_2(arguments));
#if defined CHECK_INT_OVERFLOW && defined TWOS_COMPLEMENT_INTTYPE
    if (unlikely(number == INTTYPE_MIN)) {
      /* Changing the sign of the most negative number in twos */
      /* complement arithmetic triggers an overflow.           */
      return raise_exception(SYS_OVF_EXCEPTION);
    } /* if */
#endif
    return bld_int_temp(-number);
  } /* int_negate */



/**
 *  Convert to integer.
 *  @return TRUE if the number is odd,
 *          FALSE otherwise.
 */
objectType int_odd (listType arguments)

  { /* int_odd */
    isit_int(arg_1(arguments));
    if (take_int(arg_1(arguments)) & 1) {
      return SYS_TRUE_OBJECT;
    } else {
      return SYS_FALSE_OBJECT;
    } /* if */
  } /* int_odd */



/**
 *  Compute a bitwise inclusive 'or' of two integer values.
 *  @return the bitwise inclusive 'or' of the two values.
 */
objectType int_or (listType arguments)

  { /* int_or */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    return bld_int_temp(
        take_int(arg_1(arguments)) | take_int(arg_3(arguments)));
  } /* int_or */



/**
 *  Convert to integer.
 *  @return the unchanged number.
 */
objectType int_ord (listType arguments)

  { /* int_ord */
    isit_int(arg_1(arguments));
    return bld_int_temp(take_int(arg_1(arguments)));
  } /* int_ord */



/**
 *  Compute a bitwise inclusive 'or' and assign the result back.
 */
objectType int_or_assign (listType arguments)

  {
    objectType int_variable;

  /* int_or_assign */
    int_variable = arg_1(arguments);
    isit_int(int_variable);
    is_variable(int_variable);
    isit_int(arg_3(arguments));
    int_variable->value.intValue |= take_int(arg_3(arguments));
    return SYS_EMPTY_OBJECT;
  } /* int_or_assign */



/**
 *  Convert a string to an integer number.
 *  The string must contain an integer literal consisting of an
 *  optional + or - sign, followed by a sequence of digits. Other
 *  characters as well as leading or trailing whitespace characters
 *  are not allowed. The sequence of digits is taken to be decimal.
 *  @return the integer result of the conversion.
 *  @exception RANGE_ERROR When the string is empty or it does not contain
 *             an integer literal or when the integer literal is too big
 *             or too small to be represented as integer value.
 */
objectType int_parse (listType arguments)

  { /* int_parse */
    isit_stri(arg_3(arguments));
    return bld_int_temp(
        intParse(take_stri(arg_3(arguments))));
  } /* int_parse */



/**
 *  Plus sign for integer numbers.
 *  @return its operand unchanged.
 */
objectType int_plus (listType arguments)

  { /* int_plus */
    isit_int(arg_2(arguments));
    return bld_int_temp(take_int(arg_2(arguments)));
  } /* int_plus */



/**
 *  Compute the exponentiation of a integer base with an integer exponent.
 *  @return the result of the exponentation.
 *  @exception NUMERIC_ERROR When the exponent is negative.
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_pow (listType arguments)

  { /* int_pow */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    return bld_int_temp(
#ifdef CHECK_INT_OVERFLOW
        intPowOvfChk(take_int(arg_1(arguments)), take_int(arg_3(arguments))));
#else
        intPow(take_int(arg_1(arguments)), take_int(arg_3(arguments))));
#endif
  } /* int_pow */



/**
 *  Predecessor of an integer number.
 *  @return number - 1
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_pred (listType arguments)

  {
    intType number;

  /* int_pred */
    isit_int(arg_1(arguments));
    number = take_int(arg_1(arguments));
#ifdef CHECK_INT_OVERFLOW
    if (unlikely(number == INTTYPE_MIN)) {
      return raise_exception(SYS_OVF_EXCEPTION);
    } /* if */
#endif
    return bld_int_temp(number - 1);
  } /* int_pred */



/**
 *  Convert an integer number to a string using a radix.
 *  The conversion uses the numeral system with the given base.
 *  Digit values from 10 upward are encoded with lower case letters.
 *  E.g.: 10 is encoded with a, 11 with b, etc.
 *  For negative numbers a minus sign is prepended.
 *  @return the string result of the conversion.
 *  @exception RANGE_ERROR When base < 2 or base > 36 holds.
 *  @exception MEMORY_ERROR Not enough memory to represent the result.
 */
objectType int_radix (listType arguments)

  { /* int_radix */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    return bld_stri_temp(
        intRadix(take_int(arg_1(arguments)), take_int(arg_3(arguments)), FALSE));
  } /* int_radix */



/**
 *  Convert an integer number to a string using a radix.
 *  The conversion uses the numeral system with the given base.
 *  Digit values from 10 upward are encoded with upper case letters.
 *  E.g.: 10 is encoded with A, 11 with B, etc.
 *  For negative numbers a minus sign is prepended.
 *  @return the string result of the conversion.
 *  @exception RANGE_ERROR When base < 2 or base > 36 holds.
 *  @exception MEMORY_ERROR Not enough memory to represent the result.
 */
objectType int_RADIX (listType arguments)

  { /* int_RADIX */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    return bld_stri_temp(
        intRadix(take_int(arg_1(arguments)), take_int(arg_3(arguments)), TRUE));
  } /* int_RADIX */



/**
 *  Compute pseudo-random number in the range [low, high].
 *  The random values are uniform distributed.
 *  @return a random number such that low <= rand(low, high) and
 *          rand(low, high) <= high holds.
 *  @exception RANGE_ERROR The range is empty (low > high holds).
 */
objectType int_rand (listType arguments)

  { /* int_rand */
    isit_int(arg_1(arguments));
    isit_int(arg_2(arguments));
    return bld_int_temp(
        intRand(take_int(arg_1(arguments)), take_int(arg_2(arguments))));
  } /* int_rand */



/**
 *  Compute the remainder of the integer division int_div.
 *  The remainder has the same sign as the dividend.
 *  @return the remainder of the integer division.
 *  @exception NUMERIC_ERROR When a division by zero occurs.
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_rem (listType arguments)

  {
    intType dividend;
    intType divisor;
    intType remainder;

  /* int_rem */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    dividend = take_int(arg_1(arguments));
    divisor = take_int(arg_3(arguments));
    if (unlikely(divisor == 0)) {
      return raise_exception(SYS_NUM_EXCEPTION);
#if defined CHECK_INT_OVERFLOW && defined TWOS_COMPLEMENT_INTTYPE
    } else if (unlikely(divisor == -1 && dividend == INTTYPE_MIN)) {
      /* To compute the remainder the processor does a division     */
      /* by -1. A division by -1 is equivalent to changing the      */
      /* sign. For the most negative number changing the sign       */
      /* triggers an overflow.                                      */
      return raise_exception(SYS_OVF_EXCEPTION);
#endif
    } else {
      remainder = dividend % divisor;
    } /* if */
    return bld_int_temp(remainder);
  } /* int_rem */



/**
 *  Shift an integer number right by rshift bits.
 *  A >> B is equivalent to A mdiv 2_ ** B
 *  @return the right shifted number.
 *  @exception OVERFLOW_ERROR When the shift amount is
 *             negative or greater equal INTTYPE_SIZE.
 */
objectType int_rshift (listType arguments)

  {
    intType number;
    intType rshift;
    intType result;

  /* int_rshift */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    number = take_int(arg_1(arguments));
    rshift = take_int(arg_3(arguments));
#ifdef CHECK_INT_OVERFLOW
    if (unlikely(rshift < 0 || rshift >= INTTYPE_SIZE)) {
      return raise_exception(SYS_OVF_EXCEPTION);
    } /* if */
#endif
#ifdef RSHIFT_DOES_SIGN_EXTEND
    result = number >> rshift;
#else
    if (number < 0) {
      result = ~(~number >> rshift);
    } else {
      result = number >> rshift;
    } /* if */
#endif
    return bld_int_temp(result);
  } /* int_rshift */



/**
 *  Shift a number right by rshift bits and assign the result back to number.
 *  @exception OVERFLOW_ERROR When the shift amount is
 *             negative or greater equal INTTYPE_SIZE.
 */
objectType int_rshift_assign (listType arguments)

  {
    objectType int_variable;
    intType rshift;

  /* int_rshift_assign */
    int_variable = arg_1(arguments);
    isit_int(int_variable);
    is_variable(int_variable);
    isit_int(arg_3(arguments));
    rshift = take_int(arg_3(arguments));
#ifdef CHECK_INT_OVERFLOW
    if (unlikely(rshift < 0 || rshift >= INTTYPE_SIZE)) {
      return raise_exception(SYS_OVF_EXCEPTION);
    } /* if */
#endif
#ifdef RSHIFT_DOES_SIGN_EXTEND
    int_variable->value.intValue >>= rshift;
#else
    if (take_int(arg_1(arguments)) < 0) {
      int_variable->value.intValue =
         ~(~int_variable->value.intValue >> rshift);
    } else {
      int_variable->value.intValue >>= rshift;
    } /* if */
#endif
    return SYS_EMPTY_OBJECT;
  } /* int_rshift_assign */



/**
 *  Compute the subtraction of two integer numbers.
 *  @return the difference of the two numbers.
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_sbtr (listType arguments)

  {
    intType minuend;
    intType subtrahend;

  /* int_sbtr */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    minuend = take_int(arg_1(arguments));
    subtrahend = take_int(arg_3(arguments));
#ifdef CHECK_INT_OVERFLOW
    if (subtrahend < 0) {
      if (unlikely(minuend > INTTYPE_MAX + subtrahend)) {
        return raise_exception(SYS_OVF_EXCEPTION);
      } /* if */
    } else {
      if (unlikely(minuend < INTTYPE_MIN + subtrahend)) {
        return raise_exception(SYS_OVF_EXCEPTION);
      } /* if */
    } /* if */
#endif
    return bld_int_temp(minuend - subtrahend);
  } /* int_sbtr */



/**
 *  Decrement an integer variable by a delta.
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_shrink (listType arguments)

  {
    objectType int_variable;
    intType delta;

  /* int_shrink */
    int_variable = arg_1(arguments);
    isit_int(int_variable);
    is_variable(int_variable);
    isit_int(arg_3(arguments));
    delta = take_int(arg_3(arguments));
#ifdef CHECK_INT_OVERFLOW
    {
      intType number;

      number = take_int(int_variable);
      if (delta < 0) {
        if (unlikely(number > INTTYPE_MAX + delta)) {
          return raise_exception(SYS_OVF_EXCEPTION);
        } /* if */
      } else {
        if (unlikely(number < INTTYPE_MIN + delta)) {
          return raise_exception(SYS_OVF_EXCEPTION);
        } /* if */
      } /* if */
    }
#endif
    int_variable->value.intValue -= delta;
    return SYS_EMPTY_OBJECT;
  } /* int_shrink */



/**
 *  Compute the integer square root of an integer number.
 *  @return the integer square root.
 *  @exception NUMERIC_ERROR When number is negative.
 */
objectType int_sqrt (listType arguments)

  { /* int_sqrt */
    isit_int(arg_1(arguments));
    return bld_int_temp(
        intSqrt(take_int(arg_1(arguments))));
  } /* int_sqrt */



/**
 *  Convert an integer number to a string.
 *  The number is converted to a string with decimal representation.
 *  For negative numbers a minus sign is prepended.
 *  @return the string result of the conversion.
 *  @exception MEMORY_ERROR Not enough memory to represent the result.
 */
objectType int_str (listType arguments)

  { /* int_str */
    isit_int(arg_1(arguments));
    return bld_stri_temp(
        intStr(take_int(arg_1(arguments))));
  } /* int_str */



/**
 *  Successor of an integer number.
 *  @return number + 1
 *  @exception OVERFLOW_ERROR When an integer overflow occurs.
 */
objectType int_succ (listType arguments)

  {
    intType number;

  /* int_succ */
    isit_int(arg_1(arguments));
    number = take_int(arg_1(arguments));
#ifdef CHECK_INT_OVERFLOW
    if (unlikely(number == INTTYPE_MAX)) {
      return raise_exception(SYS_OVF_EXCEPTION);
    } /* if */
#endif
    return bld_int_temp(number + 1);
  } /* int_succ */



#ifdef OUT_OF_ORDER
objectType int_uadd (listType arguments)

  { /* int_uadd */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    return bld_int_temp((intType) (
        (uintType) (take_int(arg_1(arguments))) +
        (uintType) (take_int(arg_3(arguments)))));
  } /* int_uadd */



objectType int_umult (listType arguments)

  { /* int_umult */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    return bld_int_temp((intType) (
        (uintType) (take_int(arg_1(arguments))) *
        (uintType) (take_int(arg_3(arguments)))));
  } /* int_umult */
#endif



objectType int_ulshift (listType arguments)

  {
    intType lshift;

  /* int_ulshift */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    lshift = take_int(arg_3(arguments));
#ifdef CHECK_INT_OVERFLOW
    if (unlikely(lshift < 0 || lshift >= INTTYPE_SIZE)) {
      return raise_exception(SYS_OVF_EXCEPTION);
    } /* if */
#endif
    return bld_int_temp((intType) (
        (uintType) (take_int(arg_1(arguments))) << lshift));
  } /* int_ulshift */



objectType int_ulshift_assign (listType arguments)

  {
    objectType int_variable;
    intType lshift;

  /* int_ulshift_assign */
    int_variable = arg_1(arguments);
    isit_int(int_variable);
    is_variable(int_variable);
    isit_int(arg_3(arguments));
    lshift = take_int(arg_3(arguments));
#ifdef CHECK_INT_OVERFLOW
    if (unlikely(lshift < 0 || lshift >= INTTYPE_SIZE)) {
      return raise_exception(SYS_OVF_EXCEPTION);
    } /* if */
#endif
    int_variable->value.intValue = (intType) (
        (uintType) (take_int(int_variable)) << lshift);
    return SYS_EMPTY_OBJECT;
  } /* int_ulshift_assign */



objectType int_urshift (listType arguments)

  {
    intType rshift;

  /* int_urshift */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    rshift = take_int(arg_3(arguments));
#ifdef CHECK_INT_OVERFLOW
    if (unlikely(rshift < 0 || rshift >= INTTYPE_SIZE)) {
      return raise_exception(SYS_OVF_EXCEPTION);
    } /* if */
#endif
    return bld_int_temp((intType) (
        (uintType) (take_int(arg_1(arguments))) >> rshift));
  } /* int_urshift */



objectType int_urshift_assign (listType arguments)

  {
    objectType int_variable;
    intType rshift;

  /* int_urshift_assign */
    int_variable = arg_1(arguments);
    isit_int(int_variable);
    is_variable(int_variable);
    isit_int(arg_3(arguments));
    rshift = take_int(arg_3(arguments));
#ifdef CHECK_INT_OVERFLOW
    if (unlikely(rshift < 0 || rshift >= INTTYPE_SIZE)) {
      return raise_exception(SYS_OVF_EXCEPTION);
    } /* if */
#endif
    int_variable->value.intValue = (intType) (
        (uintType) (take_int(int_variable)) >> rshift);
    return SYS_EMPTY_OBJECT;
  } /* int_urshift_assign */



#ifdef OUT_OF_ORDER
objectType int_usbtr (listType arguments)

  { /* int_usbtr */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    return bld_int_temp((intType) (
        (uintType) (take_int(arg_1(arguments))) -
        (uintType) (take_int(arg_3(arguments)))));
  } /* int_usbtr */
#endif



objectType int_value (listType arguments)

  {
    objectType obj_arg;

  /* int_value */
    isit_reference(arg_1(arguments));
    obj_arg = take_reference(arg_1(arguments));
    if (obj_arg == NULL || CATEGORY_OF_OBJ(obj_arg) != INTOBJECT) {
      return raise_exception(SYS_RNG_EXCEPTION);
    } else {
      return bld_int_temp(take_int(obj_arg));
    } /* if */
  } /* int_value */



objectType int_xor (listType arguments)

  { /* int_xor */
    isit_int(arg_1(arguments));
    isit_int(arg_3(arguments));
    return bld_int_temp(
        take_int(arg_1(arguments)) ^ take_int(arg_3(arguments)));
  } /* int_xor */



/**
 *  Compute a bitwise exclusive or ('xor') and assign the result back.
 */
objectType int_xor_assign (listType arguments)

  {
    objectType int_variable;

  /* int_xor_assign */
    int_variable = arg_1(arguments);
    isit_int(int_variable);
    is_variable(int_variable);
    isit_int(arg_3(arguments));
    int_variable->value.intValue ^= take_int(arg_3(arguments));
    return SYS_EMPTY_OBJECT;
  } /* int_xor_assign */
