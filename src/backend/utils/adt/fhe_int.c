/*-------------------------------------------------------------------------
 *
 * int8.c
 *	  Internal 64-bit integer operations
 *
 * Portions Copyright (c) 1996-2017, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/int8.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>

#include "funcapi.h"
#include "libpq/pqformat.h"
#include "utils/int8.h"
#include "utils/builtins.h"


#define MAXINT8LEN		25

#ifdef PG_MODULE_MAGIC

PG_MODULE_MAGIC;

#endif

#define MAXINT8LEN		25		

#define SAMESIGN(a,b)	(((a) < 0) == ((b) < 0))

typedef struct
{
	int64		current;
	int64		finish;
	int64		step;
} generate_series_fctx;


/***********************************************************************
 **
 **		Routines for 64-bit integers.
 **
 ***********************************************************************/

/*----------------------------------------------------------
 * Formatting and conversion routines.
 *---------------------------------------------------------*/

/*
 * scanint8 --- try to parse a string into an int8.
 *
 * If errorOK is false, ereport a useful error message if the string is bad.
 * If errorOK is true, just return "false" for bad input.
 */
bool
scanfhe_int(const char *str, bool errorOK, int64 *result)
{
	const char *ptr = str;
	int64		tmp = 0;
	int			sign = 1;

	/*
	 * Do our own scan, rather than relying on sscanf which might be broken
	 * for long long.
	 */

	/* skip leading spaces */
	while (*ptr && isspace((unsigned char) *ptr))
		ptr++;

	/* handle sign */
	if (*ptr == '-')
	{
		ptr++;

		/*
		 * Do an explicit check for INT64_MIN.  Ugly though this is, it's
		 * cleaner than trying to get the loop below to handle it portably.
		 */
		if (strncmp(ptr, "9223372036854775808", 19) == 0)
		{
			tmp = PG_INT64_MIN;
			ptr += 19;
			goto gotdigits;
		}
		sign = -1;
	}
	else if (*ptr == '+')
		ptr++;

	/* require at least one digit */
	if (!isdigit((unsigned char) *ptr))
	{
		if (errorOK)
			return false;
		else
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("invalid input syntax for integer: \"%s\"",
							str)));
	}

	/* process digits */
	while (*ptr && isdigit((unsigned char) *ptr))
	{
		int64		newtmp = tmp * 10 + (*ptr++ - '0');

		if ((newtmp / 10) != tmp)	/* overflow? */
		{
			if (errorOK)
				return false;
			else
				ereport(ERROR,
						(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
						 errmsg("value \"%s\" is out of range for type %s",
								str, "bigint")));
		}
		tmp = newtmp;
	}

gotdigits:

	/* allow trailing whitespace, but not other trailing chars */
	while (*ptr != '\0' && isspace((unsigned char) *ptr))
		ptr++;

	if (*ptr != '\0')
	{
		if (errorOK)
			return false;
		else
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("invalid input syntax for integer: \"%s\"",
							str)));
	}

	*result = (sign < 0) ? -tmp : tmp;

	return true;
}

/* fhe_int8in()
 */Datum
fhe_intin(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	int64		result;

	(void) scanint8(str, false, &result);
	PG_RETURN_INT64(result);
}


/* fhe_intout()
 */
Datum
fhe_intout(PG_FUNCTION_ARGS)
{
	int64		val = PG_GETARG_INT64(0);
	char		buf[MAXINT8LEN + 1];
	char	   *result;

	pg_lltoa(val, buf);
	result = pstrdup(buf);
	PG_RETURN_CSTRING(result);
}

/*
 *		fhe_intrecv			- converts external binary format to fhe_int
 */
Datum
fhe_intrecv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);

	PG_RETURN_INT64(pq_getmsgint64(buf));
}

/*
 *		int8send			- converts fhe_int to binary format
 */
Datum
fhe_intsend(PG_FUNCTION_ARGS)
{
	int64		arg1 = PG_GETARG_INT64(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint64(&buf, arg1);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

// Datum
// fhe_inteq(PG_FUNCTION_ARGS)
// {
// 	int64		val1 = PG_GETARG_INT64(0);
// 	int64		val2 = PG_GETARG_INT64(1);

// 	PG_RETURN_BOOL(val1 == val2);
// }

// Datum
// fhe_intne(PG_FUNCTION_ARGS)
// {
// 	int64		val1 = PG_GETARG_INT64(0);
// 	int64		val2 = PG_GETARG_INT64(1);

// 	PG_RETURN_BOOL(val1 != val2);
// }

// Datum
// fhe_intlt(PG_FUNCTION_ARGS)
// {
// 	int64		val1 = PG_GETARG_INT64(0);
// 	int64		val2 = PG_GETARG_INT64(1);

// 	PG_RETURN_BOOL(val1 < val2);
// }

// Datum
// fhe_intgt(PG_FUNCTION_ARGS)
// {
// 	int64		val1 = PG_GETARG_INT64(0);
// 	int64		val2 = PG_GETARG_INT64(1);

// 	PG_RETURN_BOOL(val1 > val2);
// }

// Datum
// fhe_intle(PG_FUNCTION_ARGS)
// {
// 	int64		val1 = PG_GETARG_INT64(0);
// 	int64		val2 = PG_GETARG_INT64(1);

// 	PG_RETURN_BOOL(val1 <= val2);
// }

// Datum
// fhe_intge(PG_FUNCTION_ARGS)
// {
// 	int64		val1 = PG_GETARG_INT64(0);
// 	int64		val2 = PG_GETARG_INT64(1);

// 	PG_RETURN_BOOL(val1 >= val2);
// }

// Datum
// fhe_intlarger(PG_FUNCTION_ARGS)
// {
// 	int64		arg1 = PG_GETARG_INT64(0);
// 	int64		arg2 = PG_GETARG_INT64(1);
// 	int64		result;

// 	result = ((arg1 > arg2) ? arg1 : arg2);

// 	PG_RETURN_INT64(result);
// }

// Datum
// fhe_intsmaller(PG_FUNCTION_ARGS)
// {
// 	int64		arg1 = PG_GETARG_INT64(0);
// 	int64		arg2 = PG_GETARG_INT64(1);
// 	int64		result;

// 	result = ((arg1 < arg2) ? arg1 : arg2);

// 	PG_RETURN_INT64(result);
// }

// Datum
// btfhe_intcmp(PG_FUNCTION_ARGS)
// {
// 	int64 b1 = PG_GETARG_INT64(0);
// 	int64 b2 = PG_GETARG_INT64(1);
// 	int32 result = 0;

// 	if (b1 < b2)
// 		result = -1;
// 	else if (b1 > b2)
// 		result = 1;
// 	else
// 		result = 0;

// 	PG_RETURN_INT32(result);
// }