#include "postgres.h"
#include "miscadmin.h" /* PostmasterPid */
#include "multimaster.h"
#include "state.h"

char const* const MtmNeighborEventMnem[] =
{
	"MTM_NEIGHBOR_CLIQUE_DISABLE",
	"MTM_NEIGHBOR_RECEIVER_START",
	"MTM_NEIGHBOR_RECOVERY_CAUGHTUP"
};

char const* const MtmEventMnem[] =
{
	"MTM_REMOTE_DISABLE",
	"MTM_CLIQUE_DISABLE",
	"MTM_CLIQUE_MINORITY",

	"MTM_ARBITER_RECEIVER_START",

	"MTM_RECOVERY_START1",
	"MTM_RECOVERY_START2",
	"MTM_RECOVERY_FINISH1",
	"MTM_RECOVERY_FINISH2",

	"MTM_WAL_RECEIVER_START",
	"MTM_WAL_SENDER_START",

	"MTM_NONRECOVERABLE_ERROR"
};


static void MtmCheckQuorum(void);
static void MtmSwitchClusterMode(MtmNodeStatus mode);


void
MtmStateProcessNeighborEvent(int node_id, MtmNeighborEvent ev)
{
	MTM_LOG1("[STATE] Processing %s", MtmNeighborEventMnem[ev]);

	switch(ev)
	{
		case MTM_NEIGHBOR_CLIQUE_DISABLE:
			MtmDisableNode(node_id);
			break;

		case MTM_NEIGHBOR_RECEIVER_START:
			/*
			* This functions is called by pglogical receiver main function when receiver background worker is started.
			* We switch to ONLINE mode when all receivers are connected.
			* As far as background worker can be restarted multiple times, use node bitmask.
			*/
			MtmLock(LW_EXCLUSIVE);
			if (!BIT_CHECK(Mtm->pglogicalReceiverMask, node_id)) {
				BIT_SET(Mtm->pglogicalReceiverMask, node_id);
				if (BIT_CHECK(Mtm->disabledNodeMask, node_id)) {
					MtmEnableNode(node_id);
				}
				MtmStateProcessEvent(MTM_WAL_RECEIVER_START);
			}
			MtmUnlock();
			break;

		case MTM_NEIGHBOR_RECOVERY_CAUGHTUP:
			Assert(BIT_CHECK(Mtm->disabledNodeMask, node_id));
			BIT_CLEAR(Mtm->originLockNodeMask, node_id);
			BIT_CLEAR(Mtm->disabledNodeMask, node_id);
			BIT_SET(Mtm->recoveredNodeMask, node_id);
			Mtm->nLiveNodes += 1;
			MtmCheckQuorum();
			break;
	}
}


void
MtmStateProcessEvent(MtmEvent ev)
{
	MTM_LOG1("[STATE] Processing %s", MtmEventMnem[ev]);

	switch (ev)
	{
		/*
		 * Arbiter response had bit turned on in disabledNodeMask for our node
		 */
		case MTM_REMOTE_DISABLE:
			if ( Mtm->status != MTM_RECOVERY &&
				 Mtm->status != MTM_RECOVERED)
			{
				// MTM_ELOG(WARNING, "Node %d thinks that I'm dead, while I'm %s (message %s)", resp->node, MtmNodeStatusMnem[Mtm->status], MtmMessageKindMnem[resp->code]);
				BIT_SET(Mtm->disabledNodeMask, MtmNodeId-1);
				Mtm->nConfigChanges += 1;
				MtmSwitchClusterMode(MTM_RECOVERY);
			}
			break;


		case MTM_CLIQUE_DISABLE:
			if (Mtm->status == MTM_ONLINE)
			{
				MtmDisableNode(MtmNodeId);
				MtmSwitchClusterMode(MTM_OFFLINE);
			}
			break;


		case MTM_CLIQUE_MINORITY:
			MtmSwitchClusterMode(MTM_IN_MINORITY);
			break;


		case MTM_ARBITER_RECEIVER_START:
			if (Mtm->nLiveNodes < Mtm->nAllNodes/2+1)
			{
				/* no quorum */
				MTM_ELOG(WARNING, "Node is out of quorum: only %d nodes of %d are accessible", Mtm->nLiveNodes, Mtm->nAllNodes);
				MtmSwitchClusterMode(MTM_IN_MINORITY);
			}
			else if (Mtm->status == MTM_INITIALIZATION)
			{
				MtmSwitchClusterMode(MTM_CONNECTED);
			}
			break;


		case MTM_RECOVERY_START1:
		case MTM_RECOVERY_START2:
			if (Mtm->status != MTM_RECOVERY)
			{
				MtmLock(LW_EXCLUSIVE);
				BIT_SET(Mtm->disabledNodeMask, MtmNodeId-1);
				Mtm->nConfigChanges += 1;
				MtmSwitchClusterMode(MTM_RECOVERY);
				Mtm->recoveredLSN = INVALID_LSN;
				MtmUnlock();
			}
			break;


		/*
		 * To avoid unnecessary simplicity we have two ways of finishing
		 * recovery:
		 *  1. Called from MtmJoinTransaction() upon receinig valid xid
		 *  2. Called from executor upon receiving 'Z' byte
		 */
		case MTM_RECOVERY_FINISH1:
		case MTM_RECOVERY_FINISH2:
			{
				int i;
				MTM_LOG1("Recovery of node %d is completed, disabled mask=%llx, connectivity mask=%llx, endLSN=%llx, live nodes=%d",
						MtmNodeId, Mtm->disabledNodeMask,
						SELF_CONNECTIVITY_MASK, (long64)GetXLogInsertRecPtr(), Mtm->nLiveNodes);
				if (Mtm->nAllNodes >= 3) {
					MTM_ELOG(WARNING, "restartLSNs at the end of recovery: {%llx, %llx, %llx}",
						Mtm->nodes[0].restartLSN, Mtm->nodes[1].restartLSN, Mtm->nodes[2].restartLSN);
				}
				MtmLock(LW_EXCLUSIVE);
				Mtm->recoverySlot = 0;
				Mtm->recoveredLSN = GetXLogInsertRecPtr();
				BIT_CLEAR(Mtm->disabledNodeMask, MtmNodeId-1);
				Mtm->nConfigChanges += 1;
				Mtm->reconnectMask |= SELF_CONNECTIVITY_MASK; /* try to reestablish all connections */
				Mtm->nodes[MtmNodeId-1].lastStatusChangeTime = MtmGetSystemTime();
				for (i = 0; i < Mtm->nAllNodes; i++) {
					Mtm->nodes[i].lastHeartbeat = 0; /* defuse watchdog until first heartbeat is received */
				}
				/* Mode will be changed to online once all logical receiver are connected */
				MTM_LOG1("[STATE] Recovery completed with %d active receivers and %d started senders from %d", Mtm->nReceivers, Mtm->nSenders, Mtm->nLiveNodes-1);
				if (Mtm->nReceivers == Mtm->nLiveNodes-1 && Mtm->nSenders == Mtm->nLiveNodes-1)
				{
					MtmSwitchClusterMode(MTM_ONLINE);
				} else {
					/* Delay switching mode to online mode and keep cluster lock to make it possible to all other nodes reestablish
					* logical replication connections with this node.
					* Under the intensive workload start of logical replication can be delayed for unpredictable amount of time
					*/
					BIT_SET(Mtm->originLockNodeMask, MtmNodeId-1); /* it is trick: this mask was originally used by WAL senders performing recovery, but here we are in opposite (recovered) side:
														* if this mask is not zero loadReq will be broadcasted to all other nodes by heartbeat, suspending their activity
														*/
					MtmSwitchClusterMode(MTM_RECOVERED);
				}
				MtmUnlock();
			}
			break;


		case MTM_WAL_RECEIVER_START:
			MTM_ELOG(LOG, "[STATE] Start %d receivers and %d senders from %d cluster status %s", Mtm->nReceivers+1, Mtm->nSenders, Mtm->nLiveNodes-1, MtmNodeStatusMnem[Mtm->status]);
			if (++Mtm->nReceivers == Mtm->nLiveNodes-1 && Mtm->nSenders == Mtm->nLiveNodes-1
				&& (Mtm->status == MTM_RECOVERED || Mtm->status == MTM_CONNECTED))
			{
				BIT_CLEAR(Mtm->originLockNodeMask, MtmNodeId-1); /* recovery is completed: release cluster lock */
				MtmSwitchClusterMode(MTM_ONLINE);
			}
			break;


		case MTM_WAL_SENDER_START:
			if (++Mtm->nSenders == Mtm->nLiveNodes-1 && Mtm->nReceivers == Mtm->nLiveNodes-1
				&& (Mtm->status == MTM_RECOVERED || Mtm->status == MTM_CONNECTED))
			{
				/* All logical replication connections from and to this node are established, so we can switch cluster to online mode */
				BIT_CLEAR(Mtm->originLockNodeMask, MtmNodeId-1); /* recovery is completed: release cluster lock */
				MtmSwitchClusterMode(MTM_ONLINE);
			}
			break;


		case MTM_NONRECOVERABLE_ERROR:
			// MTM_ELOG(WARNING, "Node is excluded from cluster because of non-recoverable error %d, %s, pid=%u",
				// edata->sqlerrcode, edata->message, getpid());
			MtmSwitchClusterMode(MTM_OUT_OF_SERVICE);
			kill(PostmasterPid, SIGQUIT);
			break;

	}

}

static void
MtmSwitchClusterMode(MtmNodeStatus mode)
{
	MTM_LOG1("[STATE] Switching status from %s to %s mode",
		MtmNodeStatusMnem[Mtm->status], MtmNodeStatusMnem[mode]);
	Mtm->status = mode;
	Mtm->nodes[MtmNodeId-1].lastStatusChangeTime = MtmGetSystemTime();
}

/*
 * Node is disabled if it is not part of clique built using connectivity masks of all nodes.
 * There is no warranty that all nodes will make the same decision about clique, but as far as we want to avoid
 * some global coordinator (which will be SPOF), we have to rely on Bron–Kerbosch algorithm locating maximum clique in graph
 */
void MtmDisableNode(int nodeId)
{
	timestamp_t now = MtmGetSystemTime();
	MTM_ELOG(WARNING, "Disable node %d at xlog position %llx, last status change time %d msec ago", nodeId, (long64)GetXLogInsertRecPtr(),
		 (int)USEC_TO_MSEC(now - Mtm->nodes[nodeId-1].lastStatusChangeTime));
	BIT_SET(Mtm->disabledNodeMask, nodeId-1);
	Mtm->nConfigChanges += 1;
	Mtm->nodes[nodeId-1].timeline += 1;
	Mtm->nodes[nodeId-1].lastStatusChangeTime = now;
	Mtm->nodes[nodeId-1].lastHeartbeat = 0; /* defuse watchdog until first heartbeat is received */
	if (nodeId != MtmNodeId) {
		Mtm->nLiveNodes -= 1;
	}
	if (Mtm->nLiveNodes >= Mtm->nAllNodes/2+1) {
		/* Make decision about prepared transaction status only in quorum */
		MtmPollStatusOfPreparedTransactionsForDisabledNode(nodeId);
	}
	MtmCheckQuorum();
}


/*
 * Node is enabled when it's recovery is completed.
 * This why node is mostly marked as recovered when logical sender/receiver to this node is (re)started.
 */
void MtmEnableNode(int nodeId)
{
	if (BIT_CHECK(Mtm->disabledNodeMask, nodeId-1)) {
		BIT_CLEAR(Mtm->disabledNodeMask, nodeId-1);
		BIT_CLEAR(Mtm->reconnectMask, nodeId-1);
		BIT_SET(Mtm->recoveredNodeMask, nodeId-1);
		Mtm->nConfigChanges += 1;
		Mtm->nodes[nodeId-1].lastStatusChangeTime = MtmGetSystemTime();
		Mtm->nodes[nodeId-1].lastHeartbeat = 0; /* defuse watchdog until first heartbeat is received */
		if (nodeId != MtmNodeId) {
			Mtm->nLiveNodes += 1;
		}
		// MTM_ELOG(WARNING, "Enable node %d at xlog position %llx", nodeId, (long64)GetXLogInsertRecPtr());
	}
	MtmCheckQuorum();
}


/*
 * Check if there is quorum: current node see more than half of all nodes
 */
static void MtmCheckQuorum(void)
{
	int nVotingNodes = MtmGetNumberOfVotingNodes();

	if (Mtm->nLiveNodes >= nVotingNodes/2+1 || (Mtm->nLiveNodes == (nVotingNodes+1)/2 && MtmMajorNode)) { /* have quorum */
		if (Mtm->status == MTM_IN_MINORITY) {
			// MTM_LOG1("Node is in majority: disabled mask %llx", Mtm->disabledNodeMask);
			MtmSwitchClusterMode(MTM_ONLINE);
		}
	} else {
		if (Mtm->status == MTM_ONLINE) { /* out of quorum */
			// MTM_ELOG(WARNING, "Node is in minority: disabled mask %llx", Mtm->disabledNodeMask);
			MtmSwitchClusterMode(MTM_IN_MINORITY);
		}
	}
}
