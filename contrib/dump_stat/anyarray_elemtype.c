#include "postgres.h"
#include "utils/array.h"


PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(anyarray_elemtype);


Datum
anyarray_elemtype(PG_FUNCTION_ARGS)
{
	#if (PG_VERSION_NUM >= 90500)
		AnyArrayType *v = PG_GETARG_ANY_ARRAY(0);
		PG_RETURN_OID(AARR_ELEMTYPE(v));
	#else
		ArrayType *v = PG_GETARG_ARRAYTYPE_P(0);
		PG_RETURN_OID(ARR_ELEMTYPE(v));
	#endif
}