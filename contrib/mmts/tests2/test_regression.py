import unittest
import subprocess
import time

class RecoveryTest(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        subprocess.check_call(['docker-compose','up',
            '--build',
            '--force-recreate',
            '-d'])

    @classmethod
    def tearDownClass(self):
        subprocess.check_call(['docker-compose','down'])

    def test_regression(self):
        # XXX: make smth clever here
        time.sleep(31)
        subprocess.check_call(['docker', 'run',
            '--network=tests2_default',
            'tests2_node1',
            '/pg/mmts/tests2/docker-regress.sh',
        ])

if __name__ == '__main__':
    unittest.main()
