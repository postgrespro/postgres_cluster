#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "ddd.h"


void MtmGraphInit(MtmGraph* graph)
{
    memset(graph->hashtable, 0, sizeof(graph->hashtable));
}

static inline MtmVertex* findVertex(MtmGraph* graph, GlobalTransactionId* gtid)
{
    xid_t h = gtid->xid % MAX_TRANSACTIONS;
    MtmVertex* v;
    for (v = graph->hashtable[h]; v != NULL; v = v->next) { 
        if (v->gtid == *gtid) { 
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
    MtmEdge *e, *next, *edges = NULL;
    while (gtid != last) { 
        Vertex* src = findVertex(graph, gtid++);
        while (gtid->node != 0) { 
            Vertex* dst = findVertex(graph, gtid++);
            e = (MtmEdge*)palloc(sizeof(MtmEdge));
            dst->nIncomingEdges += 1;
            e->dst = dst;
            e->src = src;
            e->next = v->outgoingEdges;
            v->outgoingEdges = e;
        }
		gtid += 1;
    }
}

static bool recursiveTraverseGraph(MtmVertex* root, MtmVertex* v)
{
    Edge* e;
    v->node = VISITED_NODE_MARK;
    for (e = v->outgoingEdges; e != NULL; e = e->next) {
        if (e->dst == root) { 
            return true;
        } else if (e->dst->node != VISITED_NODE_MAR && recursiveTraverseGraph(root, e->dst)) { /* loop */
            return true;
        } 
    }
    return false;        
}

bool MtmGraphFindLoop(MtmGraph* graph, GlobalTransactionId* root)
{
    Vertex* v;
    for (v = graph->hashtable[root->xid % MAX_TRANSACTIONS]; v != NULL; v = v->next) { 
        if (v->gtid == *root) { 
            if (recursiveTraverseGraph(v, v)) { 
                return true;
            }
            break;
        }
    }
    return false;        
}
