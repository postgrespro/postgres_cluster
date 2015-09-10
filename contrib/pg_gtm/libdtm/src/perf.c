#include <stdio.h>
#include <stdlib.h>

#include "../../libdtm.h"

int main() {
  cid_t gcid;
  cid_t horizon;
  DTMConn conn;
  int i;

  conn = DtmConnect("localhost", 5431);
  if (!conn) {
    fprintf(stderr, "failed to connect to dtmd\n");
    exit(1);
  }


  for (i=0; i<10000; i++) {
    horizon = DtmGlobalGetNextCid(conn);

    gcid = DtmGlobalPrepare(conn);
    if (gcid == INVALID_GCID) {
      fprintf(stderr, "failed to prepare a commit\n");
    }

    if (!DtmGlobalCommit(conn, gcid)) {
      fprintf(stderr, "failed to commit gcid = %llu\n", gcid);
    }

    if (i%100 == 0){
      printf("Completed %u txs\n", i);
    }
  }


  DtmDisconnect(conn);

  return 0;
}

