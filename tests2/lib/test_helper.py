import unittest
import time
import datetime
import psycopg2

TEST_WARMING_TIME = 5
TEST_DURATION = 10
TEST_MAX_RECOVERY_TIME = 300
TEST_RECOVERY_TIME = 30
TEST_SETUP_TIME = 20
TEST_STOP_DELAY = 5

class TestHelper(object):

    def assertIsolation(self, aggs):
        isolated = True
        for conn_id, agg in enumerate(aggs):
            isolated = isolated and agg['sumtotal']['isolation'] == 0
        if not isolated:
            raise AssertionError('Isolation failure')

    def assertCommits(self, aggs):
        commits = True
        for conn_id, agg in enumerate(aggs):
            commits = commits and 'commit' in agg['transfer']['finish']
        if not commits:
            print('No commits during aggregation interval')
            # time.sleep(100000)
            raise AssertionError('No commits during aggregation interval')

    def assertNoCommits(self, aggs):
        commits = True
        for conn_id, agg in enumerate(aggs):
            commits = commits and 'commit' in agg['transfer']['finish']
        if commits:
            raise AssertionError('There are commits during aggregation interval')

    def awaitCommit(self, node_id):
        total_sleep = 0

        while total_sleep <= TEST_MAX_RECOVERY_TIME:
            aggs = self.client.get_aggregates(clean=False, _print=False)
            # print('=== ',aggs[node_id]['transfer']['finish'])
            if ('commit' in aggs[node_id]['transfer']['finish'] and
                    aggs[node_id]['transfer']['finish']['commit'] > 10):
                break
            time.sleep(5)
            total_sleep += 5


    def performFailure(self, failure, wait=0, node_wait_for_commit=-1):

        time.sleep(TEST_WARMING_TIME)
         
        print('Simulate failure at ',datetime.datetime.utcnow())

        failure.start()

        self.client.clean_aggregates()
        print('Started failure at ',datetime.datetime.utcnow())

        time.sleep(TEST_DURATION)

        print('Getting aggs at ',datetime.datetime.utcnow())
        aggs_failure = self.client.get_aggregates()


        time.sleep(wait)
        failure.stop()

        print('Eliminate failure at ',datetime.datetime.utcnow())

        self.client.clean_aggregates()

        if node_wait_for_commit >= 0:
            self.awaitCommit(node_wait_for_commit)
        else:
            time.sleep(TEST_RECOVERY_TIME)

        aggs = self.client.get_aggregates()
        return (aggs_failure, aggs)

    def nodeExecute(dsn, statements):
        con = psycopg2.connect(dsn)
        con.autocommit = True
        cur = con.cursor()
        for statement in statements:
            cur.execute(statement)
        cur.close()
        con.close()
