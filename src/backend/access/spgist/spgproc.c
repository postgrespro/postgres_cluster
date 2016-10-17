#include "access/spgist_proc.h"
#include "utils/geo_decls.h"

/* Point-box distance in the assumption that box is aligned by axis */
double
dist_pb_simplified(Datum p, Datum b)
{
	Point *point = DatumGetPointP(p);
	BOX *box = DatumGetBoxP(b);
	double dx = 0.0, dy = 0.0;

	if (point->x < box->low.x)
		dx = box->low.x - point->x;
	if (point->x > box->high.x)
		dx = point->x - box->high.x;
	if (point->y < box->low.y)
		dy = box->low.y - point->y;
	if (point->y > box->high.y)
		dy = point->y - box->high.y;
	return HYPOT(dx,dy);
}

void
spg_point_distance(Datum to, int norderbys,
		ScanKey orderbyKeys, double **distances, bool isLeaf)
{
	double *distance;
	int sk_num;

	distance = *distances = (double *) palloc(norderbys * sizeof(double));

	for (sk_num = 0; sk_num < norderbys; ++sk_num, ++orderbyKeys, ++distance)
	{
		Datum from_point = orderbyKeys->sk_argument;
		*distance = isLeaf ? DatumGetFloat8(DirectFunctionCall2(
							   point_distance, from_point, to))
							 : dist_pb_simplified(from_point, to);
	}
}
