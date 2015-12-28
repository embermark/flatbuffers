try:
    from enum import Enum as EnumBase
except ImportError:
    # a very very very minomal subset
    Enum = object
