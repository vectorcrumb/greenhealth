import paho.mqtt.client as paho
from typing import Type, Union
from definitions import *
from devices.modules import GenericModule
from devices.vehicles import GenericVehicle
from devices.device import GenericDevice


class Manager:
    def __init__(self, mqtt_client: paho.Client):
        self._devices = {}
        self._client = mqtt_client

    def append(
        self,
        device_id: str,
        device: Union[None, Type[GenericDevice]],
    ):
        self._devices[device_id] = device
        if isinstance(device, GenericModule):
            device.spawn_nodes()
        device.subscribe_topics()
        device.acknowledge()
