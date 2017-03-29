import psycopg2
from psycopg2.extras import LogicalReplicationConnection

conn = psycopg2.connect("dbname=regression", connection_factory=LogicalReplicationConnection)
cur = conn.cursor()

cur.create_replication_slot("slotpy",
    slot_type=psycopg2.extras.REPLICATION_LOGICAL,
    output_plugin='test_decoding')

cur.start_replication("slotpy")

def consumer(msg):
    print(msg.payload)

cur.consume_stream(consumer)

