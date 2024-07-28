import paho.mqtt.client as paho
from typing import Type, Union
from devices.modules import GenericModule, RotationModule
from devices.vehicles import Rover
from devices.device import GenericDevice, DeviceType
import logging


class Manager:
    """
    The device manager is intended to hold instances to all devices
    spawned during the discovery process of a group of devices/modules.
    The device passed during an append must previously be created using
    the factory `create_device`.
    """

    def __init__(self, mqtt_client: paho.Client):
        self._devices = {}
        self._client = mqtt_client

    def append(
        self,
        device_id: str,
        device: Union[Type[GenericDevice]],
    ):
        self._devices[device_id] = device
        if isinstance(device, GenericModule):
            device.spawn_nodes()
        device.subscribe_topics()
        device.acknowledge()


def create_device(device_type: DeviceType, **kwargs):
    """
    This function is intented to be call as a factory to spawn different
    device types.
    """
    logging.debug(
        f"Creating device with device_type: {device_type} and **kwargs: {kwargs}"
    )
    match device_type:
        case DeviceType.NONE:
            return None
        case DeviceType.ROVER:
            return Rover(**kwargs)
        case DeviceType.MOD_GENERIC:
            return GenericModule(kwargs)
        case DeviceType.MOD_ROTATION:
            return RotationModule(kwargs=kwargs)
