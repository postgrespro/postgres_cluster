#ifndef SPGIST_SEARCH_H
#define	SPGIST_SEARCH_H
#include "postgres.h"

#include "access/skey.h"
#include "utils/datum.h"

extern void spg_point_distance(Datum to, int norderbys,
		ScanKey orderbyKeys, double **distances, bool isLeaf);

extern double dist_pb_simplified(Datum p, Datum b);

#endif	/* SPGIST_SEARCH_H */

