from enum import Enum
import threading
import json
import logging
from comms.mqtt_pub import MQTTPublisher, MQTTPublishable


class DeviceType(Enum):
    NONE = 0
    EMPTY = 1
    ROVER = 2
    # MOD_ are modules
    MOD_GENERIC = 3
    MOD_ROTATION = 4


class GenericDevice:
    """
    A GenericDevice is an interface for other devices to inherit from. All
    devices are considered to have the capacity of subscribing to topics and
    sending and acknowledge message to the <ID>/ack topic. From this class we
    construct GenericVehicle and GenericModule.
    """

    def __init__(self, id: str, publisher: MQTTPublisher, **kwargs) -> None:
        self._id = id
        self._pub = publisher
        self._mqtt_lock = threading.Lock()
        self.subscriptions_topics = [
            (f"{self._id}/cmd/fb", 1),
            (f"{self._id}/cmd/res", 1),
            (f"{self._id}/skills/resp", 1),
        ]

    def subscribe_topics(self):
        with self._mqtt_lock:
            for topic, qos in self.subscriptions_topics:
                logging.debug(f"Device {__name__} is subscribing to topic {topic}")
                self._pub.mqtt_client.subscribe(topic, qos)

    def acknowledge(self):
        topic_name = f"{self._id}/ack"
        payload = {"ack": True}
        self._pub.enqueue(
            MQTTPublishable(topic=topic_name, message=json.dumps(payload))
        )
