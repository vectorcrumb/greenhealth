import yaml
import json
from typing import Dict, Literal
import os
from mqtt_pub import MQTTPublisher, MQTTPublishable
import threading
from devices.device import GenericDevice


class GenericVehicle(GenericDevice):

    mappable_types = {"bool_t": bool, "int_t": int, "str_t": str, "float_t": float}

    def __init__(self, id: str, publisher: MQTTPublisher, **kwargs) -> None:
        super().__init__(id, publisher)
        self._schema = {}
        self._device_schema_path = os.path.join(
            os.path.dirname(os.path.abspath(__file__)),
            "../../config/device_schemas",
        )

    def _get_msg(self, direction: Literal["pub", "sub"], field: str) -> Dict:
        if direction not in ["pub", "sub"]:
            raise ValueError()
        try:
            msg_declaration = self._schema[f"$ID$/{field}"]
        except KeyError:
            raise ValueError(f"Field {field} doesn't exist inside schema.")
        for k, v in msg_declaration:
            if isinstance(v, str) and v.endswith("_t"):
                try:
                    msg_declaration[k] = GenericVehicle.mappable_types[v]
                except KeyError:
                    raise ValueError(f"Type {v} is not a mappable type")
        return msg_declaration

    def request_skills(self):
        topic_name = f"{self._id}/skills/req"
        self._pub.enqueue(MQTTPublishable(topic=topic_name, message=json.dumps({})))


class Rover(GenericVehicle):

    def __init__(self, id: str, publisher: MQTTPublisher, **kwargs) -> None:
        super().__init__(id, publisher, kwargs)
        with open(os.path.join(self._device_schema_path, "rover.yaml")) as schema_file:
            self._schema = yaml.safe_load(schema_file)

    # TODO: This type of functions should be autogenerated from schema?
    def cmd_move_mm(self, mm: int, timeout_ms: int = 0):
        topic_name = f"{self._id}/cmd/move_mm"
        msg_def = self._get_msg("sub", "cmd/move_mm")
        msg_def["mm"] = mm
        msg_def["timeout_ms"] = timeout_ms
        self._pub.enqueue(
            MQTTPublishable(topic=topic_name, message=json.dumps(msg_def))
        )

    def cmd_move_to_next_node(self, timeout_ms: int = 0):
        topic_name = f"{self._id}/cmd/move_to_next_node"
        msg_def = self._get_msg("sub", "cmd/move_to_next_node")
        msg_def["timeout_msg"] = timeout_ms
        self._pub.enqueue(
            MQTTPublishable(topic=topic_name, message=json.dumps(msg_def))
        )

    def cmd_move_n_nodes(self, n: int, timeout_ms: int = 0):
        topic_name = f"{self._id}/cmd/move_n_nodes"
        msg_def = self._get_msg("sub", "cmd/move_n_nodes")
        msg_def["n"] = n
        msg_def["timeout_ms"] = timeout_ms
        self._pub.enqueue(
            MQTTPublishable(topic=topic_name, message=json.dumps(msg_def))
        )
