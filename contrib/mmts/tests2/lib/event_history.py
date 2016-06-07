import time
import datetime
import uuid
from multiprocessing import Queue


class EventHistory():
    
    def __init__(self):
        self.queue = Queue()
        self.events = []
        self.running_events = {}

    def register_start(self, name):
        event_id = uuid.uuid4()
        self.queue.put({
            'name': name,
            'event_id': event_id,
            'time': datetime.datetime.now()
        })
        return event_id

    def register_finish(self, event_id, status):
        self.queue.put({
            'event_id': event_id,
            'status': status,
            'time': datetime.datetime.now()
        })

    def load_queue(self):
        while not self.queue.empty():
            event = self.queue.get()
            if 'name' in event:
                # start mark
                self.running_events[event['event_id']] = event
            else:
                # finish mark
                if event['event_id'] in self.running_events:
                    start_ev = self.running_events[event['event_id']]
                    self.events.append({
                        'name': start_ev['name'],
                        'started_at': start_ev['time'],
                        'finished_at': event['time'],
                        'status': event['status']
                    })
                    self.running_events.pop(event['event_id'], None)
                else:
                    # found finish event without corresponding start
                    raise
        return

    def aggregate(self):
        self.load_queue()

        agg = {}
        for ev in self.events:
            if ev['name'] in agg:
                named_agg = agg[ev['name']]
                latency = (ev['finished_at'] - ev['started_at']).total_seconds()
                if ev['status'] in named_agg:
                    named_agg[ev['status']] += 1
                    if named_agg['max_latency'] < latency:
                        named_agg['max_latency'] = latency
                else:
                    named_agg[ev['status']] = 0
                    named_agg['max_latency'] = latency
            else:
                agg[ev['name']] = {}

        for value in self.running_events.itervalues():
            named_agg = agg[value['name']]
            latency = (datetime.datetime.now() - ev['started_at']).total_seconds()
            if 'started' in named_agg:
                named_agg['running'] += 1
                if latency > named_agg['running_latency']:
                    named_agg['running_latency'] = latency
            else:
                named_agg['running'] = 1
                named_agg['running_latency'] = latency

        return agg

    def aggregate_by(self, period):
        return


