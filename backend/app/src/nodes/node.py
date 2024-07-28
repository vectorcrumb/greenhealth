from enum import Enum
from typing import Type
from nodes.node_catalog import GenericNode, ConnectionNode, LocationNode, TurntableNode


class NodeType(Enum):
    NONE = 0
    LOCATION = 1
    CONNECTION = 2
    TURNTABLE = 3


def create_node(node_type: NodeType, **kwargs) -> Type[GenericNode]:
    match node_type:
        case NodeType.NONE:
            raise ValueError("None is not representable as a node.")
        case NodeType.LOCATION:
            return LocationNode(kwargs)
        case NodeType.CONNECTION:
            return ConnectionNode(kwargs)
        case NodeType.TURNTABLE:
            return TurntableNode(kwargs)
