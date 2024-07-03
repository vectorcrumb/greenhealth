from enum import Enum

from devices.modules import GenericModule, RotationModule
from devices.rover import Rover


class DeviceType(Enum):
    NONE = 0
    EMPTY = 1
    ROVER = 2
    # MOD_ are modules
    MOD_GENERIC = 3
    MOD_ROTATION = 4


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
