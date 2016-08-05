#!/usr/bin/env python3
import asyncio
import uvloop
import aiopg
import random
import psycopg2
import time
import aioprocessing
import multiprocessing

class MtmTxAggregate(object):

    def __init__(self, name):
        self.name = name
        self.clear_values()

    def clear_values(self):
        self.max_latency = 0.0
        self.running_latency = 0.0
        self.finish = {}
    
    def add_finish(self, name):
        if name not in self.finish: 
            self.finish[name] = 1
        else:
            self.finish[name] += 1

class MtmClient(object):

    def __init__(self, dsns, n_accounts=100000):
        self.n_accounts = n_accounts
        self.dsns = dsns

        self.aggregates = [MtmTxAggregate('transfer'), MtmTxAggregate('transfer'), MtmTxAggregate('transfer')]
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
                self.child_pipe.send(self.aggregates)
                for aggregate in self.aggregates:
                    aggregate.clear_values()

    async def transfer(self, i):
        pool = await aiopg.create_pool(self.dsns[i])
        async with pool.acquire() as conn:
            async with conn.cursor() as cur:
                while True:
                    amount = 1
                    from_uid = random.randint(1, self.n_accounts - 1)
                    to_uid = random.randint(1, self.n_accounts - 1)

                    try:
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

                        self.aggregates[i].add_finish('commit')
                    except psycopg2.Error as e:
                        await cur.execute('rollback')
                        self.aggregates[i].add_finish(e.pgerror)

    async def total(self, i):
        pool = await aiopg.create_pool(self.dsns[i])
        async with pool.acquire() as conn:
            async with conn.cursor() as cur:
                while True:

                    try:
                        await cur.execute('select sum(amount) from bank_test')

                        if 'commit' not in self.tps_vector[i]: 
                            self.tps_vector[i]['commit'] = 1
                        else:
                            self.tps_vector[i]['commit'] += 1
                    except psycopg2.Error as e:
                        await cur.execute('rollback')
                        if e.pgerror not in self.tps_vector[i]: 
                            self.tps_vector[i][e.pgerror] = 1
                        else:
                            self.tps_vector[i][e.pgerror] += 1

    def run(self):
        asyncio.set_event_loop_policy(uvloop.EventLoopPolicy())
        self.loop = asyncio.get_event_loop()

        for i, _ in enumerate(self.dsns):
            asyncio.ensure_future(self.transfer(i))
            # asyncio.ensure_future(self.total(i))

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


c = MtmClient(['dbname=postgres user=stas host=127.0.0.1',
    'dbname=postgres user=stas host=127.0.0.1 port=5433',
    'dbname=postgres user=stas host=127.0.0.1 port=5434'], n_accounts=1000)
# c = MtmClient(['dbname=postgres user=stas host=127.0.0.1'])
c.bgrun()

while True:
    time.sleep(1)
    aggs = c.get_status()
    for agg in aggs:
        print(agg.finish)

