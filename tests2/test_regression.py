import unittest
import subprocess
import time

class RecoveryTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        subprocess.check_call(['docker-compose','up',
            '--force-recreate',
            '-d'])

    @classmethod
    def tearDownClass(self):
        subprocess.check_call(['docker-compose','down'])

    def test_regression(self):
        time.sleep(30)
        subprocess.check_call(['../../../src/test/regress/pg_regress',
            '--use-existing',
            '--schedule=../../../src/test/regress/parallel_schedule',
            '--host=127.0.0.1',
            '--port=15432',
            '--user=postgres',
            '--inputdir=../../../src/test/regress/',
            '--outputdir=../../../src/test/regress/',
            '--dlpath=/pg/src/src/test/regress/'
        ])

if __name__ == '__main__':
    unittest.main()
