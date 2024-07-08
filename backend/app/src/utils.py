import json
from typing import Dict
from definitions import *
from collections import defaultdict
import secrets
import string


def generate_random_id(length: int = 8) -> str:
    alphabet = string.ascii_lowercase + string.digits
    return "".join(secrets.choice(alphabet) for _ in range(length))


# TODO: Change this to a schema based validator!
def message_validator(json_msg: Dict, topic_name: str):
    match topic_name:
        case TOPIC_DISCOVERY:
            return set(["id", "type"]) == set(json_msg.keys())


class Graph(object):
    """Graph data structure, undirected by default."""

    def __init__(self, connections, directed=False):
        self._graph = defaultdict(set)
        self._directed = directed
        self.add_connections(connections)

    def add_connections(self, connections):
        """Add connections (list of tuple pairs) to graph"""
        for node1, node2 in connections:
            self.add(node1, node2)

    def add(self, node1, node2):
        """Add connection between node1 and node2"""
        self._graph[node1].add(node2)
        if not self._directed:
            self._graph[node2].add(node1)

    def remove(self, node):
        """Remove all references to node"""
        for n, cxns in self._graph.items():
            try:
                cxns.remove(node)
            except KeyError:
                pass
        try:
            del self._graph[node]
        except KeyError:
            pass

    def is_connected(self, node1, node2):
        """Is node1 directly connected to node2"""
        return node1 in self._graph and node2 in self._graph[node1]

    def __str__(self):
        return "{}({})".format(self.__class__.__name__, dict(self._graph))
