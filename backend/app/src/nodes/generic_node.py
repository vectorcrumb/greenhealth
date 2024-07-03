class GenericNode:
    def __init__(self, name, **kwargs) -> None:
        self.name = name
        self.__dict__.update(kwargs)

    def __repr__(self) -> str:
        return self.name
