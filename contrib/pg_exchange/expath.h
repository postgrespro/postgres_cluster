/*
 * expath.h
 *
 */

#ifndef EXPATH_H_
#define EXPATH_H_

#include "nodes/relation.h"

void force_add_path(RelOptInfo *rel, Path *path);

#endif /* EXPATH_H_ */
