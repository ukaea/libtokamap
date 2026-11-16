"""Python data sources used by the LibTokaMap Python bridge tests."""

import array
import json

import numpy as np


class EchoSource:
    """Returns its constructor arguments and the get() arguments as a JSON string."""

    def __init__(self, **kwargs):
        self.config = kwargs

    def get(self, args):
        return json.dumps({"config": self.config, "args": args}, sort_keys=True)


class ArraySource:
    """Returns the value described by args["kind"], optionally with args["dtype"]."""

    def get(self, args):
        kind = args["kind"]
        dtype = args.get("dtype", "float64")
        if kind == "arange":
            return np.arange(6, dtype=dtype).reshape(2, 3)
        if kind == "scalar":
            return np.array(7, dtype=dtype)
        if kind == "numpy_scalar":
            return np.float64(2.5)
        if kind == "empty":
            return np.zeros((0,), dtype=dtype)
        if kind == "non_contiguous":
            return np.arange(12, dtype=dtype).reshape(3, 4)[:, ::2]
        if kind == "fortran":
            return np.asfortranarray(np.arange(6, dtype=dtype).reshape(2, 3))
        if kind == "byte_swapped":
            native = np.dtype(dtype)
            swapped = native.newbyteorder("S")
            return np.arange(4, dtype=native).astype(swapped)
        if kind == "read_only":
            values = np.arange(4, dtype=dtype)
            values.setflags(write=False)
            return values
        if kind == "bool":
            return np.array([True, False, True])
        if kind == "memoryview":
            return memoryview(array.array("i", [1, 2, 3]))
        if kind == "str":
            return "héllo"
        if kind == "unicode_array":
            return np.array(["héllo"])
        if kind == "unicode_padded":
            # a single "c" stored in a <U2 array, so the element is NUL padded
            return np.array(["ab", "c"])[1:]
        if kind == "bytes_array":
            return np.array([b"abc"])
        if kind == "unicode_multi":
            return np.array(["a", "b"])
        if kind == "object":
            # e.g. a list of records, which data sources return for DIMENSION mappings to count
            return np.array([{"a": 1}, {"b": 2}, {"c": 3}], dtype=object)
        if kind == "complex":
            return np.zeros(2, dtype=np.complex128)
        if kind == "none":
            return None
        if kind == "int":
            return 3
        raise ValueError(f"unknown kind {kind}")


class DoublingSource:
    """Returns args["value"] * 2 as a one element array."""

    def get(self, args):
        return np.array([args["value"] * 2], dtype=np.int64)


class RaisingSource:
    def get(self, args):
        raise ValueError("boom from python")


class FailingConstructor:
    def __init__(self, **kwargs):
        raise RuntimeError("constructor failed")


class NoGetMethod:
    def __init__(self, **kwargs):
        pass


NOT_A_CLASS = 42


CLOSED = []


class ClosableSource:
    """Records close() calls in CLOSED; get() returns how many sources have been closed."""

    def __init__(self, name):
        self.name = name

    def get(self, args):
        return np.array([len(CLOSED)], dtype=np.int64)

    def close(self):
        CLOSED.append(self.name)


class MappingSource:
    """Data source used through MappingHandler configuration."""

    def __init__(self, scale, options):
        self.scale = scale
        self.offset = sum(options["offsets"])

    def get(self, args):
        value = args["value"]
        if isinstance(value, str):
            return value.upper()
        return np.array([value * self.scale + self.offset])


ATTACH_CALLS = []


class AttachProbe:
    """Records calls so a Python host can check the bridge used its interpreter."""

    def get(self, args):
        ATTACH_CALLS.append(args)
        return np.arange(6, dtype=np.float64)
