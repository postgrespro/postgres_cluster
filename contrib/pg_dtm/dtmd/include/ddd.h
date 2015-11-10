#ifndef DDD_H 

#include <stdbool.h>
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
    struct Vertex* next;
    xid_t xid;    
    int nIncomingEdges;
    int visited;
} Vertex;

typedef struct Graph
{
    Vertex* hashtable[MAX_TRANSACTIONS];
    Edge* freeEdges;
    Vertex* freeVertexes;
    int marker;
} Graph;


extern void initGraph(Graph* graph);
extern void addSubgraph(Instance* instance, Graph* graph, xid_t* xids, int n_xids);
extern bool findLoop(Graph* graph, xid_t root);

#endif
