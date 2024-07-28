class GenericNode:
    def __init__(self, name, **kwargs) -> None:
        self.name = name
        self.__dict__.update(kwargs)

    def __repr__(self) -> str:
        return self.name


class TurntableNode(GenericNode):
    def __init__(self, name) -> None:
        super().__init__(name)


class LocationNode(GenericNode):
    def __init__(self, name) -> None:
        super().__init__(name)


class ConnectionNode(GenericNode):
    def __init__(self, name) -> None:
        super().__init__(name)
