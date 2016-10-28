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

        # nofailure here
        # clean aggregates
        self.client.get_status()

        time.sleep(TEST_DURATION)
        aggs = self.client.get_status()

        for agg in aggs:
            self.assertTrue( aggs[agg]['finish']['commit'] > 0 )

        # nofailure ends here
        self.client.get_status()

        time.sleep(TEST_RECOVERY_TIME)
        aggs = self.client.get_status()

        for agg in aggs:
            self.assertTrue( aggs[agg]['finish']['commit'] > 0 )


    def test_node_partition(self):
        print('### nodePartitionTest ###')

        failure = SingleNodePartition('node3')

        # split one node
        failure.start()
        # clean aggregates
        self.client.get_status(print=False)

        time.sleep(TEST_DURATION)
        aggs = self.client.get_status()

        self.assertTrue( aggs['transfer_0']['finish']['commit'] > 0 )
        self.assertTrue( aggs['transfer_1']['finish']['commit'] > 0 )
        self.assertTrue( 'commit' not in aggs['transfer_2']['finish'] )
        self.assertTrue( aggs['sumtotal_0']['isolation'] == 0)
        self.assertTrue( aggs['sumtotal_1']['isolation'] == 0)
        self.assertTrue( aggs['sumtotal_2']['isolation'] == 0)

        # join splitted node
        failure.stop()
        # clear tx history
        self.client.get_status(print=False)

        time.sleep(TEST_RECOVERY_TIME)
        aggs = self.client.get_status()

        self.assertTrue( aggs['transfer_0']['finish']['commit'] > 0 )
        self.assertTrue( aggs['transfer_1']['finish']['commit'] > 0 )
        self.assertTrue( aggs['transfer_2']['finish']['commit'] > 0 )
        self.assertTrue( aggs['sumtotal_0']['isolation'] == 0)
        self.assertTrue( aggs['sumtotal_1']['isolation'] == 0)
        self.assertTrue( aggs['sumtotal_2']['isolation'] == 0)

    # def test_edge_partition(self):




if __name__ == '__main__':
    unittest.main()

