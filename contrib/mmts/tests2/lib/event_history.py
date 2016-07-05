import time
import datetime
import uuid
import copy
from multiprocessing import Queue


class EventHistory():
    
    def __init__(self):
        self.queue = Queue()
        self.events = []
        self.running_events = {}
        self.last_aggregation = datetime.datetime.now()
        self.agg_template = {
            'max_latency': 0.0,
            'running': 0,
            'running_latency': 0.0,
            'finish': {}
        }

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
                if event['event_id'] not in self.running_events:
                    # found finish event without corresponding start
                    print("ololololo!")
                    raise

                start_ev = self.running_events[event['event_id']]
                self.events.append({
                    'name': start_ev['name'],
                    'started_at': start_ev['time'],
                    'finished_at': event['time'],
                    'status': event['status']
                })
                self.running_events.pop(event['event_id'], None)

        #print(self.events)
        return

    def aggregate(self):
        self.load_queue()

        agg = {}
        for ev in self.events:
            if ev['name'] not in agg:
                agg[ev['name']] = copy.deepcopy(self.agg_template)
                #print('-=-=-', agg)

            named_agg = agg[ev['name']]
            latency = (ev['finished_at'] - ev['started_at']).total_seconds()

            if ev['status'] not in named_agg['finish']:
                named_agg['finish'][ev['status']] = 1
            else:
                named_agg['finish'][ev['status']] += 1

            if named_agg['max_latency'] < latency:
                named_agg['max_latency'] = latency

        self.events = []

        for value in self.running_events.itervalues():

            if value['name'] not in agg:
                agg[value['name']] = copy.deepcopy(self.agg_template)

            named_agg = agg[value['name']]
            latency = (datetime.datetime.now() - value['time']).total_seconds()

            named_agg['running'] += 1
            if named_agg['running_latency'] < latency:
                named_agg['running_latency'] = latency

        return agg

    def aggregate_by(self, period):
        return


