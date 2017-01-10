import unittest
import subprocess
import time

class RecoveryTest(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        print('setUp')
        subprocess.check_call(['docker-compose','up',
           '--force-recreate',
           '--build',
           '-d'])

    @classmethod
    def tearDownClass(self):
        print('tearDown')
        subprocess.check_call(['docker-compose','down'])

    def test_regression(self):
        # XXX: make smth clever here
        # time.sleep(31)
        subprocess.check_call(['docker', 'exec',
            'node1',
            '/pg/mmts/tests2/support/docker-regress.sh',
        ])

if __name__ == '__main__':
    unittest.main()
