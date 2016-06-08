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
            b = RaftClient(cs, i)
            self._clients.append(b)

    @property
    def clients(self):
        return self._clients

    def __getitem__(self, index):
        return self._clients[index]

    def start(self):
        for client in self._clients:
            client.start()

    def stop(self):
        print('collection stop called', self._clients)
        for client in self._clients:
            print('stop coll')
            client.stop()


class RaftClient(object):

    def __init__(self, connstr, node_id):
        self.connstr = connstr
        self.node_id = node_id
        self.run = Value('b', True)
        self._history = EventHistory()

    @property
    def history(self):
        return self._history

    def check(self):
        conn = psycopg2.connect(self.connstr)
        cur = conn.cursor()
        cur.execute('create extension if not exists raftable')

        while self.run.value:
            value = random.randrange(1, 1000000)

            event_id = self.history.register_start('setkey')
            cur.execute("select raftable('rush', '%d', 100)" % (value))
            print(self.node_id, 'value <- ', value)
            self.history.register_finish(event_id, 'commit')

            event_id = self.history.register_start('readkey')
            cur.execute("select raftable('rush')")
            value = cur.fetchone()[0]
            print(self.node_id, 'value -> ', value)
            self.history.register_finish(event_id, 'commit')

        cur.close()
        conn.close()

    def start(self):
        self.check_process = Process(target=self.check, args=())
        self.check_process.start()

    def stop(self):
        print('Stopping!');
        self.run.value = False
        self.check_process.join()


