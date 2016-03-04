#ifndef __DDD_H__
#define __DDD_H__

#include "multimaster.h"

#define MAX_TRANSACTIONS  1024
#define VISITED_NODE_MARK 0

typedef struct MtmEdge {
	struct MtmEdge*   next; /* list of outgoing edges */
    struct MtmVertex* dst;
    struct MtmVertex* src;
} MtmEdge;

typedef struct MtmVertex
{    
    struct MtmEdge* outgoingEdges;
    struct MtmVertex* collision;
	GlobalTransactionId gtid;
} MtmVertex;

typedef struct MtmGraph
{
    MtmVertex* hashtable[MAX_TRANSACTIONS];
} MtmGraph;

extern void MtmGraphInit(MtmGraph* graph);
extern void MtmGraphAdd(MtmGraph* graph, GlobalTransactionId* subgraph, int size);
extern bool MtmGraphFindLoop(MtmGraph* graph, GlobalTransactionId* root);

#endif
