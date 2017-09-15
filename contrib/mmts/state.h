
typedef enum
{
	MTM_NEIGHBOR_CLIQUE_DISABLE,
	MTM_NEIGHBOR_WAL_RECEIVER_START,
	MTM_NEIGHBOR_WAL_SENDER_START_RECOVERY,
	MTM_NEIGHBOR_WAL_SENDER_START_RECOVERED,
	MTM_NEIGHBOR_RECOVERY_CAUGHTUP
} MtmNeighborEvent;

typedef enum
{
	MTM_REMOTE_DISABLE,
	MTM_CLIQUE_DISABLE,
	MTM_CLIQUE_MINORITY,
	MTM_ARBITER_RECEIVER_START,
	MTM_RECOVERY_START1,
	MTM_RECOVERY_START2,
	MTM_RECOVERY_FINISH1,
	MTM_RECOVERY_FINISH2,
	MTM_NONRECOVERABLE_ERROR
} MtmEvent;

extern void MtmStateProcessNeighborEvent(int node_id, MtmNeighborEvent ev);
extern void MtmStateProcessEvent(MtmEvent ev);
extern void MtmDisableNode(int nodeId);
extern void MtmEnableNode(int nodeId);

extern void MtmOnNodeDisconnect(int nodeId);
extern void MtmOnNodeConnect(int nodeId);
extern void MtmReconnectNode(int nodeId);

extern void MtmRefreshClusterStatus(void);
