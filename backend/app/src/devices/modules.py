import yaml
from nodes.node import NodeType, create_node
import networkx as nx
import os
from comms.mqtt_pub import MQTTPublisher, MQTTPublishable
from devices.device import GenericDevice


class GenericModule(GenericDevice):
    """
    GenericModule represents an actual embeddable device which contains a
    series of nodes. Nodes are abstract locations a vehicle may be located in
    and may at times offer services. A module is described by the a device map
    which lists all nodes (with types and orientations) and the edges connecting
    said nodes. Edges are considered to be bidirectional unless indicated by
    the 'directed' argument.
    """

    def __init__(self, id: str, publisher: MQTTPublisher, **kwargs) -> None:
        super().__init__(id, publisher)
        self._map = nx.DiGraph()
        self._configuration = {}
        self._device_configs_path = os.path.join(
            os.path.dirname(os.path.abspath(__file__)),
            "../../config/device_maps",
        )

    def is_map_built(self) -> bool:
        """A map is considered built if at least one node has been defined within it

        Returns:
            bool: True if at least one node is present in the map
        """
        return bool(len(self._map.nodes))

    def spawn_nodes(self) -> bool:
        if not self._configuration:
            raise RuntimeError("No configuration loaded")
        else:
            # Parse nodes
            for node_name, node_fields in self._configuration.items():
                if node_name in ["edges", "attributes"]:
                    continue
                # TODO: Implement error checking in casting to ENUM
                node_type = NodeType[node_fields["type"].upper()]
                node = NodeFactory.create_node(node_type, name=node_name)
                self._map.add_node(node_name, node=node)
            # Parse edges
            for edge in self._configuration["edges"]:
                self._map.add_edge(
                    edge["source"], edge["target"], distance=edge["distance"]
                )
                # If not directed, create inverse bidirectional connection
                if not edge.get("directed", False):
                    self._map.add_edge(
                        edge["target"], edge["source"], distance=edge["distance"]
                    )
            # If there is an attributes field, add to module
            if "attributes" in self._configuration:
                self.__dict__.update(self._configuration["attributes"])


class RotationModule(GenericModule):

    def __init__(self, id: str, publisher: MQTTPublisher, **kwargs) -> None:
        super().__init__(id, publisher)
        with open(
            os.path.join(self._device_configs_path, "turntable.yaml")
        ) as configuration_file:
            self._configuration = yaml.safe_load(configuration_file)
