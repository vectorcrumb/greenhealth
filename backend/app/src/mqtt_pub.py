import threading
import paho.mqtt.client as paho
import queue
from typing import Any
import sentinel

ThreadEnd = sentinel.create("ThreadEnd")

class MQTTPublisher(threading.Thread):
    def __init__(self, mqtt_client: paho.Client):
        super().__init__()
        self._queue = queue.Queue()
        self.mqtt_client = mqtt_client
        self.daemon = True
    
    def enqueue(self, msg: Any):
        self._queue.put(msg)

    def teardown(self):
        self._queue.put(ThreadEnd)

    def run(self):
        while True:
            qelem = self.queue.get()
            print(f"Queue elem: {qelem}")
            if qelem is ThreadEnd:
                break
