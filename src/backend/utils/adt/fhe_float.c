/*-------------------------------------------------------------------------
 *
 * float.c
 *	  Functions for the built-in floating-point types.
 *
 * Portions Copyright (c) 1996-2017, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/float.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <ctype.h>
#include <float.h>
#include <math.h>
#include <limits.h>

#include "catalog/pg_type.h"
#include "libpq/pqformat.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/sortsupport.h"


// #ifndef M_PI
/* from my RH5.2 gcc math.h file - thomas 2000-04-03 */


/*
 * Returns -1 if 'val' represents negative infinity, 1 if 'val'
 * represents (positive) infinity, and 0 otherwise. On some platforms,
 * this is equivalent to the isinf() macro, but not everywhere: C99
 * does not specify that isinf() needs to distinguish between positive
 * and negative infinity.

 *		fhe_floatin		- converts "num" to fhe_float
 */
Datum
fhe_floatin(PG_FUNCTION_ARGS)
{
	char	   *num = PG_GETARG_CSTRING(0);

	PG_RETURN_FLOAT8(float8in_internal(num, NULL, "double precision", num));
}

/*
 * float8in_internal - guts of float8in()
 *
 * This is exposed for use by functions that want a reasonably
 * platform-independent way of inputting doubles.  The behavior is
 * essentially like strtod + ereport on error, but note the following
 * differences:
 * 1. Both leading and trailing whitespace are skipped.
 * 2. If endptr_p is NULL, we throw error if there's trailing junk.
 * Otherwise, it's up to the caller to complain about trailing junk.
 * 3. In event of a syntax error, the report mentions the given type_name
 * and prints orig_string as the input; this is meant to support use of
 * this function with types such as "box" and "point", where what we are
 * parsing here is just a substring of orig_string.
 *
 * "num" could validly be declared "const char *", but that results in an
 * unreasonable amount of extra casting both here and in callers, so we don't.
//  */
// double
// fhe_floatin_internal(char *num, char **endptr_p,
// 				  const char *type_name, const char *orig_string)
// {
// 	double		val;
// 	char	   *endptr;

// 	/* skip leading whitespace */
// 	while (*num != '\0' && isspace((unsigned char) *num))
// 		num++;

// 	/*
// 	 * Check for an empty-string input to begin with, to avoid the vagaries of
// 	 * strtod() on different platforms.
// 	 */
// 	if (*num == '\0')
// 		ereport(ERROR,
// 				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
// 				 errmsg("invalid input syntax for type %s: \"%s\"",
// 						type_name, orig_string)));

// 	errno = 0;
// 	val = strtod(num, &endptr);

// 	/* did we not see anything that looks like a double? */
// 	if (endptr == num || errno != 0)
// 	{
// 		int			save_errno = errno;

// 		/*
// 		 * C99 requires that strtod() accept NaN, [+-]Infinity, and [+-]Inf,
// 		 * but not all platforms support all of these (and some accept them
// 		 * but set ERANGE anyway...)  Therefore, we check for these inputs
// 		 * ourselves if strtod() fails.
// 		 *
// 		 * Note: C99 also requires hexadecimal input as well as some extended
// 		 * forms of NaN, but we consider these forms unportable and don't try
// 		 * to support them.  You can use 'em if your strtod() takes 'em.
// 		 */
// 		if (pg_strncasecmp(num, "NaN", 3) == 0)
// 		{
// 			val = get_fhe_float_nan();
// 			endptr = num + 3;
// 		}
// 		else if (pg_strncasecmp(num, "Infinity", 8) == 0)
// 		{
// 			val = get_fhe_float_infinity();
// 			endptr = num + 8;
// 		}
// 		else if (pg_strncasecmp(num, "+Infinity", 9) == 0)
// 		{
// 			val = get_fhe_float_infinity();
// 			endptr = num + 9;
// 		}
// 		else if (pg_strncasecmp(num, "-Infinity", 9) == 0)
// 		{
// 			val = -get_fhe_float_infinity();
// 			endptr = num + 9;
// 		}
// 		else if (pg_strncasecmp(num, "inf", 3) == 0)
// 		{
// 			val = get_fhe_float_infinity();
// 			endptr = num + 3;
// 		}
// 		else if (pg_strncasecmp(num, "+inf", 4) == 0)
// 		{
// 			val = get_fhe_float_infinity();
// 			endptr = num + 4;
// 		}
// 		else if (pg_strncasecmp(num, "-inf", 4) == 0)
// 		{
// 			val = -get_fhe_float_infinity();
// 			endptr = num + 4;
// 		}
// 		else if (save_errno == ERANGE)
// 		{
// 			/*
// 			 * Some platforms return ERANGE for denormalized numbers (those
// 			 * that are not zero, but are too close to zero to have full
// 			 * precision).  We'd prefer not to throw error for that, so try to
// 			 * detect whether it's a "real" out-of-range condition by checking
// 			 * to see if the result is zero or huge.
// 			 *
// 			 * On error, we intentionally complain about double precision not
// 			 * the given type name, and we print only the part of the string
// 			 * that is the current number.
// 			 */
// 			if (val == 0.0 || val >= HUGE_VAL || val <= -HUGE_VAL)
// 			{
// 				char	   *errnumber = pstrdup(num);

// 				errnumber[endptr - num] = '\0';
// 				ereport(ERROR,
// 						(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
// 						 errmsg("\"%s\" is out of range for type double precision",
// 								errnumber)));
// 			}
// 		}
// 		else
// 			ereport(ERROR,
// 					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
// 					 errmsg("invalid input syntax for type %s: \"%s\"",
// 							type_name, orig_string)));
// 	}
// #ifdef HAVE_BUGGY_SOLARIS_STRTOD
// 	else
// 	{
// 		/*
// 		 * Many versions of Solaris have a bug wherein strtod sets endptr to
// 		 * point one byte beyond the end of the string when given "inf" or
// 		 * "infinity".
// 		 */
// 		if (endptr != num && endptr[-1] == '\0')
// 			endptr--;
// 	}
// #endif							/* HAVE_BUGGY_SOLARIS_STRTOD */

// 	/* skip trailing whitespace */
// 	while (*endptr != '\0' && isspace((unsigned char) *endptr))
// 		endptr++;

// 	/* report stopping point if wanted, else complain if not end of string */
// 	if (endptr_p)
// 		*endptr_p = endptr;
// 	else if (*endptr != '\0')
// 		ereport(ERROR,
// 				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
// 				 errmsg("invalid input syntax for type %s: \"%s\"",
// 						type_name, orig_string)));

// 	return val;
// }

// /*
//  *		fhe_floatout		- converts fhe_float number to a string
//  *						  using a standard output format
//  */
Datum
fhe_floatout(PG_FUNCTION_ARGS)
{
	float8		num = PG_GETARG_FLOAT8(0);

	PG_RETURN_CSTRING(float8out_internal(num));
}

/*
 * float8out_internal - guts of float8out()
 *
 * This is exposed for use by functions that want a reasonably
 * platform-independent way of outputting doubles.
 * The result is always palloc'd.
//  */
// char *
// fhe_floatout_internal(double num)
// {
// 	char	   *ascii = (char *) palloc(MAXDOUBLEWIDTH + 1);

// 	if (isnan(num))
// 		return strcpy(ascii, "NaN");

// 	switch (is_infinite(num))
// 	{
// 		case 1:
// 			strcpy(ascii, "Infinity");
// 			break;
// 		case -1:
// 			strcpy(ascii, "-Infinity");
// 			break;
// 		default:
// 			{
// 				int			ndig = DBL_DIG + extra_float_digits;

// 				if (ndig < 1)
// 					ndig = 1;

// 				snprintf(ascii, MAXDOUBLEWIDTH + 1, "%.*g", ndig, num);
// 			}
// 	}

// 	return ascii;
// }

// /*
//  *		fhe_floatrecv			- converts external binary format to fhe_float
//  */
Datum
fhe_floatrecv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);

	PG_RETURN_FLOAT8(pq_getmsgfloat8(buf));
}

/*
 *		float8send			- converts float8 to binary format
 */
Datum
fhe_floatsend(PG_FUNCTION_ARGS)
{
	float8		num = PG_GETARG_FLOAT8(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendfloat8(&buf, num);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}
Datum
fhe_floateq(PG_FUNCTION_ARGS)
{
	float8		arg1 = PG_GETARG_FLOAT8(0);
	float8		arg2 = PG_GETARG_FLOAT8(1);

	PG_RETURN_BOOL(float8_cmp_internal(arg1, arg2) == 0);
}

// Datum
// fhe_floatabs(PG_FUNCTION_ARGS)
// {
// 	float8		arg1 = PG_GETARG_FLOAT8(0);

// 	PG_RETURN_FLOAT8(fabs(arg1));
// }


// /*
//  *		float8um		- returns -arg1 (unary minus)
//  */
// Datum
// fhe_floatum(PG_FUNCTION_ARGS)
// {
// 	float8		arg1 = PG_GETARG_FLOAT8(0);
// 	float8		result;

// 	result = -arg1;
// 	PG_RETURN_FLOAT8(result);
// }

// Datum
// fhe_floatup(PG_FUNCTION_ARGS)
// {
// 	float8		arg = PG_GETARG_FLOAT8(0);

// 	PG_RETURN_FLOAT8(arg);
// }

// Datum
// fhe_floatlarger(PG_FUNCTION_ARGS)
// {
// 	float8		arg1 = PG_GETARG_FLOAT8(0);
// 	float8		arg2 = PG_GETARG_FLOAT8(1);
// 	float8		result;

// 	if (float8_cmp_internal(arg1, arg2) > 0)
// 		result = arg1;
// 	else
// 		result = arg2;
// 	PG_RETURN_FLOAT8(result);
// }

// Datum
// fhe_floatsmaller(PG_FUNCTION_ARGS)
// {
// 	float8		arg1 = PG_GETARG_FLOAT8(0);
// 	float8		arg2 = PG_GETARG_FLOAT8(1);
// 	float8		result;

// 	if (float8_cmp_internal(arg1, arg2) < 0)
// 		result = arg1;
// 	else
// 		result = arg2;
// 	PG_RETURN_FLOAT8(result);
// }
