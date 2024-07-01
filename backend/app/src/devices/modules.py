import yaml
from nodes.node import NodeFactory, NodeType
import networkx as nx
import os

class GenericModule:

    def __init__(self, **kwargs) -> None:
        self._map = nx.DiGraph()
        self._configuration = {}
        self._device_configs_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../config/device_configurations")

    def is_map_built(self) -> bool:
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
                node_type = NodeType[node_fields['type'].upper()]
                node = NodeFactory.create_node(node_type, name=node_name)
                self._map.add_node(node_name, node=node)
            # Parse edges
            for edge in self._configuration['edges']:
                self._map.add_edge(edge['source'], edge['target'], distance=edge['distance'])
                # If not directed, create inverse bidirectional connection
                if not edge.get("directed", False):
                    self._map.add_edge(edge['target'], edge['source'], distance=edge['distance'])
            # If there is an attributes field, add to module
            if "attributes" in self._configuration:
                self.__dict__.update(self._configuration['attributes'])


class RotationModule(GenericModule):

    def __init__(self, **kwargs) -> None:
        super().__init__()
        with open(os.path.join(self._device_configs_path, "turntable.yaml")) as configuration_file:
            self._configuration = yaml.safe_load(configuration_file)
