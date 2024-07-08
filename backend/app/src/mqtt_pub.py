import threading
import paho.mqtt.client as paho
import queue
from typing import Any, Dict
import sentinel
import logging
from dataclasses import dataclass


ThreadEnd = sentinel.create("ThreadEnd")


@dataclass
class MQTTPublishable:
    topic: str
    message: str
    qos: int = 1


class MQTTPublisher(threading.Thread):
    def __init__(self, mqtt_client: paho.Client):
        super().__init__()
        self._queue = queue.Queue()
        self.mqtt_client = mqtt_client
        self.daemon = True
        self.mqtt_lock = threading.Lock()
        logging.debug("MQTT Publisher thread ready")

    def _enqueue(self, msg: Any):
        self._queue.put(msg)

    def enqueue(self, msg: MQTTPublishable):
        if not isinstance(msg, MQTTPublishable):
            raise ValueError("Passed msg argument is not an MQTTPublishable type")
        self._queue.put(msg)

    def teardown(self):
        self._queue.put(ThreadEnd)

    def run(self):
        while True:
            qelem = self._queue.get()
            logging.debug(f"Queue elem: {qelem}")
            if qelem is ThreadEnd:
                break
            if isinstance(qelem, MQTTPublishable):
                with self.mqtt_lock:
                    self.mqtt_client.publish(qelem.topic, qelem.message, qelem.qos)
