import unittest
import time
import subprocess
from lib.bank_client import *

class RecoveryTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.clients = ClientCollection([
            "dbname=postgres host=127.0.0.1 user=postgres",
            "dbname=postgres host=127.0.0.1 user=postgres port=5433",
            "dbname=postgres host=127.0.0.1 user=postgres port=5434"
        ])
        self.clients.start()
        time.sleep(5)

    @classmethod
    def tearDownClass(self):
        print('tearDown')
        self.clients.stop()

    # def test_normal_operations(self):
    #     print('### normalOpsTest ###')

    #     for i in range(3):
    #         time.sleep(3)
    #         aggs = self.clients.aggregate()
    #         for agg in aggs:
    #             # there were some commits
    #             self.assertTrue( agg['transfer'] > 0 )

    def test_node_prtition(self):
        print('### nodePartitionTest ###')

        subprocess.check_call(['blockade','partition','node3'])
        print('### blockade node3 ###')

        # clear tx history
        self.clients.aggregate(echo=False)

        for i in range(10):
            time.sleep(3)
            aggs = self.clients.aggregate()
            #self.assertTrue( aggs[0]['transfer']['finish']['Commit'] > 0 )
            #self.assertTrue( aggs[1]['transfer']['finish']['Commit'] > 0 )
            #self.assertTrue( 'Commit' not in aggs[2]['transfer']['finish'] )

        subprocess.check_call(['blockade','join'])
        print('### deblockade node3 ###')

        # clear tx history
        self.clients.aggregate(echo=False)

        for i in range(1000):
            time.sleep(3)
            aggs = self.clients.aggregate()
            print(i, aggs)


if __name__ == '__main__':
    unittest.main()

