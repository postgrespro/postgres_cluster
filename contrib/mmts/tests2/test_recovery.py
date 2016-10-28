#
# Based on Aphyr's test for CockroachDB.
#

import unittest
import time
import subprocess
import datetime
import docker

from lib.bank_client import MtmClient
from lib.failure_injector import *

TEST_DURATION = 10
TEST_RECOVERY_TIME = 10

class RecoveryTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        # subprocess.check_call(['docker-compose','up',
        #     '--force-recreate',
        #     '-d'])

        # XXX: add normal wait here
        # time.sleep(30)
        self.client = MtmClient([
            "dbname=regression user=postgres host=127.0.0.1 port=15432",
            "dbname=regression user=postgres host=127.0.0.1 port=15433",
            "dbname=regression user=postgres host=127.0.0.1 port=15434"
        ], n_accounts=1000)
        self.client.bgrun()

    @classmethod
    def tearDownClass(self):
        print('tearDown')
        self.client.stop()
        # XXX: check nodes data identity here
        # subprocess.check_call(['docker-compose','down'])

    def test_normal_operations(self):
        print('### normal_operations ###')

        self.client.clean_aggregates()
        time.sleep(TEST_DURATION)
        aggs_failure = self.client.get_aggregates()

        self.client.clean_aggregates()
        time.sleep(TEST_RECOVERY_TIME)
        aggs = self.client.get_aggregates()

        for agg in aggs_failure:
            self.assertTrue( 'commit' in aggs_failure[agg]['finish'] )

        for agg in aggs:
            self.assertTrue( 'commit' in aggs[agg]['finish'] )


    def test_node_partition(self):
        print('### nodePartitionTest ###')

        failure = SingleNodePartition('node3')
        failure.start()

        self.client.clean_aggregates()
        time.sleep(TEST_DURATION)
        aggs_failure = self.client.get_aggregates()

        failure.stop()

        self.client.clean_aggregates()
        time.sleep(TEST_RECOVERY_TIME)
        aggs = self.client.get_aggregates()

        self.assertTrue( 'commit' in aggs_failure['transfer_0']['finish'] )
        self.assertTrue( 'commit' in aggs_failure['transfer_1']['finish'] )
        self.assertTrue( 'commit' not in aggs_failure['transfer_2']['finish'] )
        self.assertTrue( aggs_failure['sumtotal_0']['isolation'] == 0)
        self.assertTrue( aggs_failure['sumtotal_1']['isolation'] == 0)
        self.assertTrue( aggs_failure['sumtotal_2']['isolation'] == 0)

        self.assertTrue( 'commit' in aggs['transfer_0']['finish'] )
        self.assertTrue( 'commit' in aggs['transfer_1']['finish'] )
        self.assertTrue( 'commit' in aggs['transfer_2']['finish'] )
        self.assertTrue( aggs['sumtotal_0']['isolation'] == 0)
        self.assertTrue( aggs['sumtotal_1']['isolation'] == 0)
        self.assertTrue( aggs['sumtotal_2']['isolation'] == 0)


    def test_node_partition(self):
        print('### nodePartitionTest ###')

        failure = SingleNodePartition('node3')
        failure.start()

        self.client.clean_aggregates()
        time.sleep(TEST_DURATION)
        aggs_failure = self.client.get_aggregates()

        failure.stop()

        self.client.clean_aggregates()
        time.sleep(TEST_RECOVERY_TIME)
        aggs = self.client.get_aggregates()

        self.assertTrue( 'commit' in aggs_failure['transfer_0']['finish'] )
        self.assertTrue( 'commit' in aggs_failure['transfer_1']['finish'] )
        self.assertTrue( 'commit' not in aggs_failure['transfer_2']['finish'] )
        self.assertTrue( aggs_failure['sumtotal_0']['isolation'] == 0)
        self.assertTrue( aggs_failure['sumtotal_1']['isolation'] == 0)
        self.assertTrue( aggs_failure['sumtotal_2']['isolation'] == 0)

        self.assertTrue( 'commit' in aggs['transfer_0']['finish'] )
        self.assertTrue( 'commit' in aggs['transfer_1']['finish'] )
        self.assertTrue( 'commit' in aggs['transfer_2']['finish'] )
        self.assertTrue( aggs['sumtotal_0']['isolation'] == 0)
        self.assertTrue( aggs['sumtotal_1']['isolation'] == 0)
        self.assertTrue( aggs['sumtotal_2']['isolation'] == 0)


    def test_edge_partition(self):
        print('### edgePartitionTest ###')

        failure = EdgePartition('node2', 'node3')
        failure.start()

        self.client.clean_aggregates()
        time.sleep(TEST_DURATION)
        aggs_failure = self.client.get_aggregates()

        failure.stop()

        self.client.clean_aggregates()
        time.sleep(TEST_RECOVERY_TIME)
        aggs = self.client.get_aggregates()

        self.assertTrue( ('commit' in aggs_failure['transfer_0']['finish']) or ('commit' in aggs_failure['transfer_1']['finish']) )
        self.assertTrue( 'commit' not in aggs_failure['transfer_2']['finish'] )
        self.assertTrue( aggs_failure['sumtotal_0']['isolation'] == 0)
        self.assertTrue( aggs_failure['sumtotal_1']['isolation'] == 0)
        self.assertTrue( aggs_failure['sumtotal_2']['isolation'] == 0)

        self.assertTrue( 'commit' in aggs['transfer_0']['finish'] )
        self.assertTrue( 'commit' in aggs['transfer_1']['finish'] )
        self.assertTrue( 'commit' in aggs['transfer_2']['finish'] )
        self.assertTrue( aggs['sumtotal_0']['isolation'] == 0)
        self.assertTrue( aggs['sumtotal_1']['isolation'] == 0)
        self.assertTrue( aggs['sumtotal_2']['isolation'] == 0)

if __name__ == '__main__':
    unittest.main()

