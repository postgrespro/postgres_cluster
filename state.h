
typedef enum
{
	MTM_NEIGHBOR_CLIQUE_DISABLE,
	MTM_NEIGHBOR_RECEIVER_START,
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
	MTM_WAL_RECEIVER_START,
	MTM_WAL_SENDER_START,
	MTM_NONRECOVERABLE_ERROR
} MtmEvent;

extern void MtmStateProcessNeighborEvent(int node_id, MtmNeighborEvent ev);
extern void MtmStateProcessEvent(MtmEvent ev);
extern void MtmDisableNode(int nodeId);
extern void MtmEnableNode(int nodeId);

