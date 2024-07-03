from enum import Enum
from typing import Union
from nodes.generic_node import GenericNode
from nodes.connection import ConnectionNode
from nodes.location import LocationNode
from nodes.turntable import TurntableNode


class NodeType(Enum):
    NONE = 0
    LOCATION = 1
    CONNECTION = 2
    TURNTABLE = 3


class NodeFactory:
    def create_node(node_type: NodeType, **kwargs) -> Union[GenericNode, None]:
        match node_type:
            case NodeType.NONE:
                raise ValueError("None is not representable as a node.")
            case NodeType.LOCATION:
                return LocationNode(kwargs)
            case NodeType.CONNECTION:
                return ConnectionNode(kwargs)
            case NodeType.TURNTABLE:
                return TurntableNode(kwargs)
