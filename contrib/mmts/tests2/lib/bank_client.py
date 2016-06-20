import psycopg2
import random
from multiprocessing import Process, Value, Queue
import time
import sys
from event_history import *

class ClientCollection(object):
    def __init__(self, connstrs):
        self._clients = []

        for i, cs in enumerate(connstrs):
            b = BankClient(cs, i)
            self._clients.append(b)

        self._clients[0].initialize()

    @property
    def clients(self):
        return self._clients

    def __getitem__(self, index):
        return self._clients[index]

    def start(self):
        for client in self._clients:
            client.start()

    def stop(self):
        for client in self._clients:
            client.stop()


class BankClient(object):

    def __init__(self, connstr, node_id):
        self.connstr = connstr
        self.node_id = node_id
        self.run = Value('b', True)
        self._history = EventHistory()
        self.accounts = 10000
        self.show_errors = True

    def initialize(self):
        conn = psycopg2.connect(self.connstr)
        cur = conn.cursor()
        cur.execute('create extension if not exists multimaster')
        conn.commit()

        cur.execute('create table bank_test(uid int primary key, amount int)')

        cur.execute('''
                insert into bank_test
                select *, 0 from generate_series(0, %s)''',
                (self.accounts,))
        conn.commit()
        cur.close()
        conn.close()

    @property
    def history(self):
        return self._history

    def print_error(self, arg, comment=''):
        if self.show_errors:
            print('Node', self.node_id, 'got error', arg, comment)

    def check_total(self):
        conn, cur = self.connect()

        while self.run.value:
            event_id = self.history.register_start('total')

            try:
                cur.execute('select sum(amount) from bank_test')
                res = cur.fetchone()
                if res[0] != 0:
                    print("Isolation error, total = %d" % (res[0],))
                    raise BaseException
            except psycopg2.InterfaceError:
                conn, cur = self.connect(reconnect=True)
            except:
                self.print_error(sys.exc_info(),'3')
                self.history.register_finish(event_id, 'rollback')
            else:
                self.history.register_finish(event_id, 'commit')

        cur.close()
        conn.close()

    def transfer_money(self):
        #conn = psycopg2.connect(self.connstr)
        #cur = conn.cursor()
        conn, cur = self.connect()

        i = 0
        while self.run.value:
            i += 1
            amount = 1
            from_uid = random.randrange(1, self.accounts + 1)
            to_uid = random.randrange(1, self.accounts + 1)

            event_id = self.history.register_start('transfer')

            try:
                cur.execute('''update bank_test
                    set amount = amount - %s
                    where uid = %s''',
                    (amount, from_uid))
                cur.execute('''update bank_test
                    set amount = amount + %s
                    where uid = %s''',
                    (amount, to_uid))

                conn.commit()
            except psycopg2.InterfaceError:
                conn, cur = self.connect(reconnect=True)
            except:
                self.print_error(sys.exc_info(),'1')
                self.history.register_finish(event_id, 'rollback')
            else:
                self.history.register_finish(event_id, 'commit')

        cur.close()
        conn.close()

    def connect(self, reconnect=False):
        
        while True:
            try:
                conn = psycopg2.connect(self.connstr)
                cur = conn.cursor()
                return conn, cur
            except:
                self.print_error(sys.exc_info(),'2')
                if not reconnect:
                    raise
                if not self.run.value:
                    raise

    # def watchdog(self):
    #    while self.run.value:
    #        time.sleep(1)
    #        print('watchdog: ', self.history.aggregate())

    def start(self):
        self.transfer_process = Process(target=self.transfer_money, args=())
        self.transfer_process.start()

        self.total_process = Process(target=self.check_total, args=())
        self.total_process.start()

        #self.total_process = Process(target=self.watchdog, args=())
        #self.total_process.start()

        return

    def stop(self):
        print('Stopping!');
        self.run.value = False
        self.total_process.terminate()
        self.transfer_process.terminate()
        return

    def cleanup(self):
        conn = psycopg2.connect(self.connstr)
        cur = conn.cursor()
        cur.execute('drop table bank_test')
        conn.commit()
        cur.close()
        conn.close()

