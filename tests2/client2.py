#!/usr/bin/env python3
import asyncio
# import uvloop
import aiopg
import random
import psycopg2
import time
import datetime
import copy
import aioprocessing
import multiprocessing

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
            'running_latency': (datetime.datetime.now() - self.start_time).total_seconds(),
            'max_latency': self.max_latency,
            'isolation': self.isolation,
            'finish': copy.deepcopy(self.finish)
        }

class MtmClient(object):

    def __init__(self, dsns, n_accounts=100000):
        self.n_accounts = n_accounts
        self.dsns = dsns
        self.aggregates = {}
        self.initdb()
        self.running = True

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
                serialized_aggs = {}
                for name, aggregate in self.aggregates.items():
                    serialized_aggs[name] = aggregate.as_dict() 
                    aggregate.clear_values()
                self.child_pipe.send(serialized_aggs)


    @asyncio.coroutine
    def exec_tx(self, tx_block, aggname_prefix, conn_i):
        aggname = "%s_%i" % (aggname_prefix, conn_i)
        agg = self.aggregates[aggname] = MtmTxAggregate(aggname)
        pool = yield from aiopg.create_pool(self.dsns[conn_i])
        conn = yield from pool.acquire()
        cur = yield from conn.cursor()
        while self.running:
            agg.start_tx()
            try:
                yield from cur.execute('commit')
                yield from tx_block(conn, cur)
                agg.finish_tx('commit')
            except psycopg2.Error as e:
                agg.finish_tx(e.pgerror)

    @asyncio.coroutine
    def transfer_tx(self, conn, cur):
        amount = 1
        # to avoid deadlocks:
        from_uid = random.randint(1, self.n_accounts - 2)
        to_uid = from_uid + 1
        yield from cur.execute('begin')
        yield from cur.execute('''update bank_test
            set amount = amount - %s
            where uid = %s''',
            (amount, from_uid))
        yield from cur.execute('''update bank_test
            set amount = amount + %s
            where uid = %s''',
            (amount, to_uid))
        yield from cur.execute('commit')

    @asyncio.coroutine
    def total_tx(self, conn, cur):
        yield from cur.execute('select sum(amount) from bank_test')
        total = yield from cur.fetchone()
        if total[0] != 0:
            self.isolation += 1

    def run(self):
        # asyncio.set_event_loop_policy(uvloop.EventLoopPolicy())
        self.loop = asyncio.get_event_loop()

        for i, _ in enumerate(self.dsns):
            asyncio.async(self.exec_tx(self.transfer_tx, 'transfer', i))
            asyncio.async(self.exec_tx(self.total_tx, 'sumtotal', i))

        asyncio.async(self.status())

        self.loop.run_forever()
    
    def bgrun(self):
        print('Starting evloop in different process');
        self.parent_pipe, self.child_pipe = aioprocessing.AioPipe()
        self.evloop_process = multiprocessing.Process(target=self.run, args=())
        self.evloop_process.start()

    def get_status(self):
        self.parent_pipe.send('status')
        return self.parent_pipe.recv()
    
    def stop(self):
        self.running = False
        self.evloop_process.terminate()

    @classmethod
    def print_aggregates(cls, serialized_agg):
            columns = ['running_latency', 'max_latency', 'isolation', 'finish']

            # print table header
            print("\t\t", end="")
            for col in columns:
                print(col, end="\t")
            print("\n", end="")

            serialized_agg

            for aggname in sorted(serialized_agg.keys()):
                agg = serialized_agg[aggname]
                print("%s\t" % aggname, end="")
                for col in columns:
                    if col in agg:
                        if isinstance(agg[col], float):
                            print("%.2f\t" % (agg[col],), end="\t")
                        else:
                            print(agg[col], end="\t")
                    else:
                        print("-\t", end="")
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
