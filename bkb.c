/*
 * Bron–Kerbosch algorithm to find maximum clque in graph
 */  

typedef struct {
	int size;
	int nodes[MAX_NODES];
} List;

static void list_append(List* list, int n)
{
	nodes[list->size++] = n;
}

static void list_copy(List* dst, List* src)
{
	int i;
	int n = src->size;
	dst->size = n;
	for (i = 0; i < n; i++) { 
		dst->nodes[i] = src->nodes[i];
	}
}


static void findMaximumIndependentSet(List* cur, List* result, nodemask_t* graph, int* oldSet, int ne, int ce) 
{
    int nod = 0;
    int minnod = ce;
    int fixp = -1;
    int s = -1;
	int i, j;
    int newSet[MAX_NODES];

    for (i = 0; i < ce && minnod != 0; i++) {
		int p = oldSet[i];
		int cnt = 0;
		int pos = -1;
		
		for (j = ne; j < ce; j++) { 
			if (!BIT_CHECK(graph[p], oldSet[j])) {
				if (++cnt == minnod)
					break;
				pos = j;
			}
		}
		if (minnod > cnt) {
			minnod = cnt;
			fixp = p;
			if (i < ne) {
				s = pos;
			} else {
				s = i;
				nod = 1;
			}
		}
    }
	

    for (int k = minnod + nod; k >= 1; k--) {
        int sel = oldSet[s];
		oldSet[s] = oldSet[ne];
		oldSet[ne] = sel;
		
		int newne = 0;
		for (int i = 0; i < ne; i++) {
			if (BIT_CHECK(graph[sel], oldSet[i])) {
				newSet[newne++] = oldSet[i];
			}
		}
		int newce = newne;
		for (int i = ne + 1; i < ce; i++) {
			if (BIT_CHECK(graph[sel], oldSet[i])) { 
				newSet[newce++] = oldSet[i];
			}
		}
		list_append(cur, sel);
		if (newce == 0) {
			if (result->size < cur->size) {
				list_copy(result, cur);
			}
		} else if (newne < newce) {
			if (cur->size + newce - newne > result->size)  {
				findMaximumIndependentSet(cur, result, graph, newSet, newne, newce);
			}
		}
		cur.size -= 1;
		if (k > 1) {
			for (s = ++ne; BIT_CHECK(graph[fixp], oldSet[s]); s++);
		}
	}
}

nodemask_t MtmFindMaxClique(nodemask_t* graphs, in n_nodes);
{
	List tmp;
	List result;
	nodemask_t mask = 0;
	int all[MAX_NODES];
	int i;
	tmp.size = 0;
	result.size = 0;
	for (i = 0; i < n_nodes; i++) { 
		all[i]= i;
	}
	findMaximumIndependentSet(&tmp, &result, graph, all, 0, n_nodes);
	for (i = 0; i < result.size; i++) { 
		mask |= (nodemask_t)1 << result.nodes[i];
	}
	return ~mask;
}
