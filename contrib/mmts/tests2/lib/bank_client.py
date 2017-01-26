#!/usr/bin/env python3
import asyncio
# import uvloop
import aiopg
import random
import psycopg2
from psycopg2.extensions import *
import time
import datetime
import copy
import aioprocessing
import multiprocessing
import logging

class MtmTxAggregate(object):

    def __init__(self, name):
        self.name = name
        self.isolation = 0
        self.clear_values()

    def clear_values(self):
        self.max_latency = 0.0
        self.finish = {}

    def start_tx(self):
        self.start_time = datetime.datetime.now()

    def finish_tx(self, name):
        latency = (datetime.datetime.now() - self.start_time).total_seconds()

        if latency > self.max_latency:
            self.max_latency = latency

        if name not in self.finish:
            self.finish[name] = 1
        else:
            self.finish[name] += 1

    def as_dict(self):
        return {
            'running_latency': 'xxx', #(datetime.datetime.now() - self.start_time).total_seconds(),
            'max_latency': self.max_latency,
            'isolation': self.isolation,
            'finish': copy.deepcopy(self.finish)
        }

def keep_trying(tries, delay, method, name, *args, **kwargs):
    for t in range(tries):
        try:
            return method(*args, **kwargs)
        except Exception as e:
            if t == tries - 1:
                raise Exception("%s failed all %d tries" % (name, tries)) from e
            print("%s failed [%d of %d]: %s" % (name, t + 1, tries, str(e)))
            time.sleep(delay)
    raise Exception("this should not happen")

class MtmClient(object):

    def __init__(self, dsns, n_accounts=100000):
        # logging.basicConfig(level=logging.DEBUG)
        self.n_accounts = n_accounts
        self.dsns = dsns
        self.aggregates = {}
        keep_trying(40, 1, self.initdb, 'self.initdb')
        self.running = True
        self.nodes_state_fields = ["id", "disabled", "disconnected", "catchUp", "slotLag",
            "avgTransDelay", "lastStatusChange", "oldestSnapshot", "SenderPid",
            "SenderStartTime ", "ReceiverPid", "ReceiverStartTime", "connStr"]
        self.oops = '''
                        . . .                         
                         \|/                          
                       `--+--'                        
                         /|\                          
                        ' | '                         
                          |                           
                          |                           
                      ,--'#`--.                       
                      |#######|                       
                   _.-'#######`-._                    
                ,-'###############`-.                 
              ,'#####################`,               
             /#########################\              
            |###########################|             
           |#############################|            
           |#############################|            
           |#############################|            
           |#############################|            
            |###########################|             
             \#########################/              
              `.#####################,'               
                `._###############_,'                 
                   `--..#####..--'      
'''

    def initdb(self):
        conn = psycopg2.connect(self.dsns[0])
        cur = conn.cursor()
        cur.execute('create extension if not exists multimaster')
        conn.commit()
        cur.execute('drop table if exists bank_test')
        cur.execute('create table bank_test(uid int primary key, amount int)')
        cur.execute('''
                insert into bank_test
                select *, 0 from generate_series(0, %s)''',
                (self.n_accounts,))
        conn.commit()
        cur.close()
        conn.close()

    @asyncio.coroutine
    def status(self):
        while self.running:
            msg = yield from self.child_pipe.coro_recv()
            if msg == 'status':
                serialized_aggs = []

                for conn_id, conn_aggs in self.aggregates.items():
                    serialized_aggs.append({})
                    for aggname, agg in conn_aggs.items():
                        serialized_aggs[conn_id][aggname] = agg.as_dict()
                        agg.clear_values()

                self.child_pipe.send(serialized_aggs)
            else:
                print('evloop: unknown message')

    @asyncio.coroutine
    def exec_tx(self, tx_block, aggname_prefix, conn_i):
        aggname = "%s_%i" % (aggname_prefix, conn_i)
        if conn_i not in self.aggregates:
            self.aggregates[conn_i] = {}
        agg = self.aggregates[conn_i][aggname_prefix] = MtmTxAggregate(aggname)
        dsn = self.dsns[conn_i]

        conn = cur = False

        while self.running:
            agg.start_tx()

            try:
                if (not conn) or conn.closed:
                        # enable_hstore tries to perform select from database
                        # which in case of select's failure will lead to exception
                        # and stale connection to the database
                        conn = yield from aiopg.connect(dsn, enable_hstore=False)
                        print("reconnected")

                if (not cur) or cur.closed:
                        cur = yield from conn.cursor()

                # ROLLBACK tx after previous exception.
                # Doing this here instead of except handler to stay inside try
                # block.
                status = yield from conn.get_transaction_status()
                if status != TRANSACTION_STATUS_IDLE:
                    yield from cur.execute('rollback')

                yield from tx_block(conn, cur, agg)
                agg.finish_tx('commit')

            except psycopg2.Error as e:
                agg.finish_tx(str(e).strip())
                # Give evloop some free time.
                # In case of continuous excetions we can loop here without returning
                # back to event loop and block it
                yield from asyncio.sleep(0.01)
            except BaseException as e:
                print('Catch exception: ', e)
                agg.finish_tx(str(e).strip())
                # Give evloop some free time.
                # In case of continuous excetions we can loop here without returning
                # back to event loop and block it
                yield from asyncio.sleep(0.01)                

        print("We've count to infinity!")

    @asyncio.coroutine
    def transfer_tx(self, conn, cur, agg):
        amount = 1
        # to avoid deadlocks:
        from_uid = random.randint(1, self.n_accounts - 2)
        to_uid = from_uid + 1
        yield from cur.execute('begin')
        yield from cur.execute('''update bank_test
            set amount = amount - %s
            where uid = %s''',
            (amount, from_uid))
        assert(cur.rowcount == 1)
        yield from cur.execute('''update bank_test
            set amount = amount + %s
            where uid = %s''',
            (amount, to_uid))
        assert(cur.rowcount == 1)
        yield from cur.execute('commit')

    @asyncio.coroutine
    def total_tx(self, conn, cur, agg):
        yield from cur.execute('select sum(amount) from bank_test')
        total = yield from cur.fetchone()
        if total[0] != 0:
            agg.isolation += 1
            print(self.oops)
            print('Isolation error, total = ', total[0])
            # yield from cur.execute('select * from mtm.get_nodes_state()')
            # nodes_state = yield from cur.fetchall()
            # for i, col in enumerate(self.nodes_state_fields):
            #     print("%17s" % col, end="\t")
            #     for j in range(3):
            #          print("%19s" % nodes_state[j][i], end="\t")
            #     print("\n")

    def run(self):
        # asyncio.set_event_loop_policy(uvloop.EventLoopPolicy())
        self.loop = asyncio.get_event_loop()

        for i, _ in enumerate(self.dsns):
            asyncio.ensure_future(self.exec_tx(self.transfer_tx, 'transfer', i))
            asyncio.ensure_future(self.exec_tx(self.total_tx, 'sumtotal', i))

        asyncio.ensure_future(self.status())

        self.loop.run_forever()

    def bgrun(self):
        print('Starting evloop in different process')
        self.parent_pipe, self.child_pipe = aioprocessing.AioPipe()
        self.evloop_process = multiprocessing.Process(target=self.run, args=())
        self.evloop_process.start()

    def get_aggregates(self, _print=True):
        self.parent_pipe.send('status')
        resp = self.parent_pipe.recv()
        if _print:
            MtmClient.print_aggregates(resp)
        return resp

    def clean_aggregates(self):
        self.parent_pipe.send('status')
        self.parent_pipe.recv()

    def stop(self):
        self.running = False
        self.evloop_process.terminate()

    @classmethod
    def print_aggregates(cls, aggs):
            columns = ['running_latency', 'max_latency', 'isolation', 'finish']

            # print table header
            print("\t\t", end="")
            for col in columns:
                print(col, end="\t")
            print("\n", end="")

            for conn_id, agg_conn in enumerate(aggs):
                for aggname, agg in agg_conn.items():
                    print("Node %d: %s\t" % (conn_id + 1, aggname), end="")
                    for col in columns:
                        if isinstance(agg[col], float):
                            print("%.2f\t" % (agg[col],), end="\t")
                        else:
                            print(agg[col], end="\t")
                    print("")
            print("")

if __name__ == "__main__":
    c = MtmClient(['dbname=postgres user=postgres host=127.0.0.1',
        'dbname=postgres user=postgres host=127.0.0.1 port=5433',
        'dbname=postgres user=postgres host=127.0.0.1 port=5434'], n_accounts=10000)
    c.bgrun()
    while True:
        time.sleep(1)
        aggs = c.get_status()
        MtmClient.print_aggregates(aggs)
