import unittest
import time
import subprocess
from lib.bank_client import *


class RecoveryTest(unittest.TestCase):
    def setUp(self):
        #subprocess.check_call(['blockade','up'])
        self.clients = ClientCollection([
            "dbname=postgres host=127.0.0.1 user=postgres",
            "dbname=postgres host=127.0.0.1 user=postgres port=5433",
            "dbname=postgres host=127.0.0.1 user=postgres port=5434"
        ])
        self.clients.start()

    def tearDown(self):
        print('tearDown')
        self.clients.stop()
        self.clients[0].cleanup()
        subprocess.check_call(['blockade','join'])

    def test_0_normal_operation(self):
        print('### normalOpsTest ###')
        print('Waiting 5s to check operability')
        time.sleep(5)
        
        for client in self.clients:
            agg = client.history.aggregate()
            print(agg)
            self.assertTrue(agg['transfer']['commit'] > 0)

    def test_1_node_disconnect(self):
        print('### disconnectTest ###')

        subprocess.check_call(['blockade','partition','node3'])
        print('Node3 disconnected')

        print('Waiting 20s to discover failure')
        time.sleep(20)
        for client in self.clients:
            agg = client.history.aggregate()
            print(agg)

#        print('Waiting 3s to check operability')
#        time.sleep(3)
#        for client in self.clients:
#            agg = client.history.aggregate()
#            print(agg)
#
#        subprocess.check_call(['blockade','join'])
#        print('Node3 connected back')
#
#        print('Waiting 12s for catch-up')
#        time.sleep(12)
#
#        for client in self.clients:
#            agg = client.history.aggregate()
#            print(agg)


if __name__ == '__main__':
    unittest.main()



