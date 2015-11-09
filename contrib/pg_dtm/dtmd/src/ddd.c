#include "transaction.h"

typedef struct Instance {
    struct Edge* edges; /* local subgraph */    
} Instance;

typedef struct Edge {
    L2List node; /* node of list of outgoing eedges */
    struct Edge* next;  /* list of edges of local subgraph */  
    struct Vertex* dst;
    struct Vertex* src;
} Edge;

typedef struct Vertex
{
    L2List outgoingEdges;
    xid_t xid;    
    int nIncomingEdges;
    bool visited;
} Vertex;

typedef struct Graph
{
    Vertex* hashtable[MAX_TRANSACTIONS];
    Edge* freeEdges;
    Vertex* freeVertexes;
} Graph;

void initGraph(Graph* graph)
{
    memset(graph->hashtable, 0. sizeof(graph->hashtable));
    graph->freeEdges = NULL;
    graph->freeVertexes = NULL;
}

Edge* newEdge(Graph* graph)
{
    Edge* edge = graph->freeEdges;
    if (edge == NULL) { 
        edge = (Edge*)malloc(sizeof(Edge));
    } else { 
        graph->freeEdges = edge->next;
    }
    return edge;
}

void freeVertex(Graph* graph, Vertex* vertex)
{
    vertex->node.next = (L2List*)graph->freeVertexes;
    graph->freeVertexes = vertex;
}

void freeEdge(Graph* graph, Edge* edge)
{
    edge->next = graph->freeEdges;
    graph->freeEdges = edge;
}

Vertex* newVertex(Graph* graph)
{
    Vertex* v = graph->freeVertexes;
    if (v == NULL) { 
        v = (Vertex*)malloc(sizeof(Vertex));
    } else { 
        graph->freeVertexes = (Vertex*)v.node.next;
    }
    return v;
}

Vertex* findVertex(Graph* graph, xid_t xid)
{
    xid_t h = xid;
    Vertex* v;
    while ((v = graph->hashtable[h % MAX_TRANSACTIONS]) != NULL) { 
        if (v->xid == xid) { 
            return v;
        }
        h += 1;
    }
    v = newVertex(graph);
    l2_list_init(v->outgoingEdges);
    v->xid = xid;
    v->nIncomingEdges = 0;
    graph->hashtable[h % MAX_TRANSACTIONS] = v;
    return v;
}

void addSubgraph(Instance* instance, Graph* graph, xid_t* xids, int n_xids)
{
    xid_t *last = xids + n_xids;
    Edge *e, *next, *edges = NULL;
    while (xids != last) { 
        Vertex* src = findVertex(graph, *xids++);
        xid_t xid;
        while ((xid = *xids++) != 0) { 
            Vertex* dst = findVertex(graph, xid);
            e = newEdge(graph);
            dst->nIncomingEdges += 1;
            e->dst = dst;
            e->src = src;
            e->next = edges;
            edges = e;
            l2_list_link(&src->outgoingEdges, &e->node);
        }
    }
    for (e = instance->edges; e != NULL; e = next) { 
        next = e->next;
        l2_list_unlink(&e->node);
        if (--e->dst->nIncomingEdges == 0 && l2_list_is_empty(&e->dst->outgoingEdges)) {
            freeVertex(e->dst);
        }
        if (e->src->nIncomingEdges == 0 && l2_list_is_empty(&e->src->outgoingEdges)) {
            freeVertex(e->src);
        }
        freeEdge(e);
    }
}

bool findLoop(Graph* graph)
{
}
