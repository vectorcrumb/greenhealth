import paho.mqtt.client as paho
from typing import Type, Union
from definitions import *
from devices.modules import GenericModule
from devices.rover import Rover

class Manager:
    def __init__(self, mqtt_client: paho.Client):
        self._devices = {}
        self._client = mqtt_client
    
    def append(self, device_id: str, device: Union[None, Rover, Type[GenericModule]]):
        self._devices[device_id] = device
        if isinstance(device, GenericModule):
            device.spawn_nodes()
        for topic, qos in LIST_SUB_TOPICS_NEW_CONNECTION:
            self._client.subscribe(topic.format(id=device_id), qos)