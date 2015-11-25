#ifndef DDD_H 

#include <stdbool.h>
#include "transaction.h"

typedef struct Node {
    struct Node* collision;
    struct Edge* edges; /* local subgraph */    
    nodeid_t node_id;
} Node;

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
    int deadlock_duration;
} Vertex;

typedef struct Graph
{
    Vertex* hashtable[MAX_TRANSACTIONS];
    Edge* freeEdges;
    Vertex* freeVertexes;
    int marker;
    int min_deadlock_duration;
} Graph;

typedef struct Cluster 
{
    Node* hashtable[MAX_STREAMS];
} Cluster;

extern void initGraph(Graph* graph);
extern void addSubgraph(Graph* graph, nodeid_t node_id, xid_t* xids, int n_xids);
extern bool detectDeadLock(Graph* graph, xid_t root);

#endif
