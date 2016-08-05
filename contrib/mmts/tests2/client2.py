#!/usr/bin/env python3
import asyncio
import uvloop
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

    async def status(self):
        while True:
            msg = await self.child_pipe.coro_recv()
            if msg == 'status':
                serialized_aggs = {}
                for name, aggregate in self.aggregates.items():
                    serialized_aggs[name] = aggregate.as_dict() 
                    aggregate.clear_values()
                self.child_pipe.send(serialized_aggs)

    async def exec_tx(self, tx_block, aggname_prefix, conn_i):
        aggname = "%s_%i" % (aggname_prefix, conn_i)
        agg = self.aggregates[aggname] = MtmTxAggregate(aggname)

        pool = await aiopg.create_pool(self.dsns[conn_i])
        async with pool.acquire() as conn:
            async with conn.cursor() as cur:
                while True:
                    agg.start_tx()
                    try:
                        await tx_block(conn, cur)
                        agg.finish_tx('commit')
                    except psycopg2.Error as e:
                        await cur.execute('rollback')
                        agg.finish_tx(e.pgerror)

    async def transfer_tx(self, conn, cur):
        amount = 1
        # to avoid deadlocks:
        from_uid = random.randint(1, self.n_accounts - 2)
        to_uid = from_uid + 1
        await cur.execute('begin')
        await cur.execute('''update bank_test
            set amount = amount - %s
            where uid = %s''',
            (amount, from_uid))
        await cur.execute('''update bank_test
            set amount = amount + %s
            where uid = %s''',
            (amount, to_uid))
        await cur.execute('commit')                        

    async def total_tx(self, conn, cur):
        await cur.execute('select sum(amount) from bank_test')
        total = await cur.fetchone()
        if total[0] != 0:
            self.isolation_errors += 1

    def run(self):
        asyncio.set_event_loop_policy(uvloop.EventLoopPolicy())
        self.loop = asyncio.get_event_loop()

        for i, _ in enumerate(self.dsns):
            asyncio.ensure_future(self.exec_tx(self.transfer_tx, 'transfer', i))
            asyncio.ensure_future(self.exec_tx(self.total_tx, 'sumtotal', i))

        asyncio.ensure_future(self.status())

        self.loop.run_forever()
    
    def bgrun(self):
        print('Starting evloop in different process');
        self.parent_pipe, self.child_pipe = aioprocessing.AioPipe()
        self.evloop_process = multiprocessing.Process(target=self.run, args=())
        self.evloop_process.start()

    def get_status(self):
        c.parent_pipe.send('status')
        return c.parent_pipe.recv()

def print_aggregates(serialized_agg):
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

c = MtmClient(['dbname=postgres user=stas host=127.0.0.1',
    'dbname=postgres user=stas host=127.0.0.1 port=5433',
    'dbname=postgres user=stas host=127.0.0.1 port=5434'], n_accounts=10000)
c.bgrun()

while True:
    time.sleep(1)
    aggs = c.get_status()
    print_aggregates(aggs)
    # for k, v in aggs.items():
        # print(k, v.finish)

