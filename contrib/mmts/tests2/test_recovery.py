import unittest
import time
import subprocess
# from lib.bank_client import *
from client2 import MtmClient

class RecoveryTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.client = MtmClient([
            "dbname=postgres user=postgres host=127.0.0.1",
            "dbname=postgres user=postgres host=127.0.0.1 port=5433",
            "dbname=postgres user=postgres host=127.0.0.1 port=5434"
        ])
        self.client.bgrun()
        time.sleep(5)

    @classmethod
    def tearDownClass(self):
        print('tearDown')
        self.client.stop()

    # def test_normal_operations(self):
    #     print('### normalOpsTest ###')

    #     for i in range(3):
    #         time.sleep(3)
    #         aggs = self.clients.aggregate()
    #         for agg in aggs:
    #             # there were some commits
    #             self.assertTrue( agg['transfer'] > 0 )

    def test_node_partition(self):
        print('### nodePartitionTest ###')

        subprocess.check_call(['blockade','partition','node3'])
        print('### blockade node3 ###')

        # clear tx history
        self.client.get_status()

        for i in range(10):
            print(i)
            time.sleep(3)
            aggs = self.client.get_status()
            MtmClient.print_aggregates(aggs)
            #self.assertTrue( aggs[0]['transfer']['finish']['Commit'] > 0 )
            #self.assertTrue( aggs[1]['transfer']['finish']['Commit'] > 0 )
            #self.assertTrue( 'Commit' not in aggs[2]['transfer']['finish'] )

        subprocess.check_call(['blockade','join'])
        print('### deblockade node3 ###')

        # clear tx history
        self.client.get_status()

        for i in range(30):
            print(i)
            time.sleep(3)
            aggs = self.client.get_status()
            MtmClient.print_aggregates(aggs)


if __name__ == '__main__':
    unittest.main()

