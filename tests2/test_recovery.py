import unittest
import time
import subprocess
from lib.bank_client import *

class RecoveryTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        #subprocess.check_call(['blockade','up'])
        self.clients = ClientCollection([
            "dbname=postgres host=127.0.0.1 user=postgres",
            "dbname=postgres host=127.0.0.1 user=postgres port=5433",
            "dbname=postgres host=127.0.0.1 user=postgres port=5434"
        ])

    @classmethod
    def tearDownClass(self):
        print('tearDown')
        #subprocess.check_call(['blockade','join'])

        # in case of error
        self.clients.stop()
        #self.clients[0].cleanup()


    def test_0_normal_operation(self):
        print('### normalOpsTest ###')

        for i in range(1000):
            time.sleep(3)
            self.clients.print_agg()

    def test_2_node_disconnect(self):
        print('### disconnectTest ###')

        self.clients.set_acc_to_tx(10000)
        self.clients.start()

        subprocess.check_call(['blockade','partition','node3'])
        print('Node3 disconnected')

        # give cluster some time to discover problem
        time.sleep(3)

        for i in range(5):
            time.sleep(3)
            for client in self.clients:
                agg = client.history.aggregate()
                print(agg)
                self.assertTrue(agg['transfer']['finish']['Commit'] > 0)
            print("\n")

        subprocess.check_call(['blockade','join'])
        self.clients.stop()

if __name__ == '__main__':
    unittest.main()

