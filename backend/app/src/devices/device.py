from enum import Enum
import threading
import json

from devices.modules import GenericModule, RotationModule
from devices.vehicles import Rover
from mqtt_pub import MQTTPublisher, MQTTPublishable


class DeviceType(Enum):
    NONE = 0
    EMPTY = 1
    ROVER = 2
    # MOD_ are modules
    MOD_GENERIC = 3
    MOD_ROTATION = 4


class GenericDevice:

    def __init__(self, id: str, publisher: MQTTPublisher) -> None:
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
            for topic, qos in self.subscribe_topics:
                self._pub.mqtt_client.subscribe(topic, qos)

    def acknowledge(self):
        topic_name = f"{self._id}/ack"
        payload = {"ack": True}
        self._pub.enqueue(
            MQTTPublishable(topic=topic_name, message=json.dumps(payload))
        )


class DeviceFactory:
    def create_device(device_type: DeviceType, **kwargs):
        print(kwargs)
        match device_type:
            case DeviceType.NONE:
                return None
            case DeviceType.ROVER:
                return Rover(kwargs)
            case DeviceType.MOD_GENERIC:
                return GenericModule(kwargs)
            case DeviceType.MOD_ROTATION:
                return RotationModule(kwargs=kwargs)
