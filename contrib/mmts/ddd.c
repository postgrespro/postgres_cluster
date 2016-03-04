#include "postgres.h"
#include "access/clog.h"
#include "storage/lwlock.h"
#include "utils/hsearch.h"

#include "ddd.h"


void MtmGraphInit(MtmGraph* graph)
{
    memset(graph->hashtable, 0, sizeof(graph->hashtable));
}

static inline MtmVertex* findVertex(MtmGraph* graph, GlobalTransactionId* gtid)
{
    uint32 h = gtid->xid % MAX_TRANSACTIONS;
    MtmVertex* v;
    for (v = graph->hashtable[h]; v != NULL; v = v->collision) { 
        if (EQUAL_GTID(v->gtid, *gtid)) { 
            return v;
        }
    }
	v = (MtmVertex*)palloc(sizeof(MtmVertex));
    v->gtid = *gtid;
	v->outgoingEdges = NULL;
    v->collision = graph->hashtable[h];
    graph->hashtable[h] = v;
    return v;
}

void MtmGraphAdd(MtmGraph* graph, GlobalTransactionId* gtid, int size)
{
    GlobalTransactionId* last = gtid + size;
    while (gtid != last) { 
        MtmVertex* src = findVertex(graph, gtid++);
        while (gtid->node != 0) { 
            MtmVertex* dst = findVertex(graph, gtid++);
            MtmEdge* e = (MtmEdge*)palloc(sizeof(MtmEdge));
            e->dst = dst;
            e->src = src;
            e->next = src->outgoingEdges;
            src->outgoingEdges = e;
        }
		gtid += 1;
    }
}

static bool recursiveTraverseGraph(MtmVertex* root, MtmVertex* v)
{
    MtmEdge* e;
    v->gtid.node = VISITED_NODE_MARK;
    for (e = v->outgoingEdges; e != NULL; e = e->next) {
        if (e->dst == root) { 
            return true;
        } else if (e->dst->gtid.node != VISITED_NODE_MARK && recursiveTraverseGraph(root, e->dst)) { /* loop */
            return true;
        } 
    }
    return false;        
}

bool MtmGraphFindLoop(MtmGraph* graph, GlobalTransactionId* root)
{
    MtmVertex* v;
    for (v = graph->hashtable[root->xid % MAX_TRANSACTIONS]; v != NULL; v = v->collision) { 
        if (EQUAL_GTID(v->gtid, *root)) { 
            if (recursiveTraverseGraph(v, v)) { 
                return true;
            }
            break;
        }
    }
    return false;        
}
