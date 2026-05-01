from __future__ import annotations

import json
from collections.abc import Callable
from typing import Any

import numpy as np
import pytest

import libtokamap
from helpers import JSONDataSource, REPO_ROOT, copy_example_mappings, dot_product, write_config


Callback = Callable[[dict[str, libtokamap.MappedValue], dict[str, Any]], object]


def make_custom_mapper(tmp_path, callback: Callback) -> libtokamap.Mapper:
    mapping_dir = copy_example_mappings(tmp_path)
    mappings_path = mapping_dir / "example_v1/magnetics/40/mappings.json"
    mappings = json.loads(mappings_path.read_text(encoding="utf-8"))
    mappings["coil[#]/flux/callback_result"] = {
        "map_type": "CUSTOM",
        "library": "custom",
        "function": "callback_result",
        "inputs": {},
        "parameters": {},
    }
    mappings_path.write_text(json.dumps(mappings), encoding="utf-8")

    config_path = write_config(tmp_path / "config.toml", mapping_dir, REPO_ROOT / "examples/simple_mapper/schemas")
    mapper = libtokamap.Mapper(str(config_path))
    mapper.register_python_data_source("JSON", JSONDataSource(REPO_ROOT / "examples/simple_mapper/data"))
    mapper.register_custom_function("custom", "dot_product", dot_product)
    mapper.register_custom_function("custom", "callback_result", callback)
    return mapper


@pytest.mark.parametrize(
    ("dtype", "values"),
    [
        (np.bool_, [True, False, True]),
        (np.int8, [65, 66, 67]),
        (np.int16, [-2, 0, 2]),
        (np.int32, [-3, 0, 3]),
        (np.int64, [-4, 0, 4]),
        (np.uint8, [0, 1, 2]),
        (np.uint16, [0, 2, 4]),
        (np.uint32, [0, 3, 6]),
        (np.uint64, [0, 4, 8]),
        (np.float32, [0.0, 0.25, 0.5]),
        (np.float64, [0.0, 0.5, 1.0]),
    ],
)
def test_custom_callback_accepts_supported_numpy_dtypes(tmp_path, dtype, values) -> None:
    expected = np.array(values, dtype=dtype)

    def callback(_inputs: dict[str, libtokamap.MappedValue], _params: dict[str, Any]) -> np.ndarray:
        return expected

    mapper = make_custom_mapper(tmp_path, callback)
    result = mapper.map("example", "magnetics/coil[0]/flux/callback_result", {"shot": 42})

    if expected.dtype == np.dtype(np.bool_):
        assert isinstance(result, np.ndarray)
        assert result.dtype == np.dtype("uint8")
        np.testing.assert_array_equal(result, expected.astype(np.uint8))
    elif expected.dtype == np.dtype(np.int8):
        assert result == "ABC"
    else:
        assert isinstance(result, np.ndarray)
        assert result.dtype == expected.dtype
        np.testing.assert_array_equal(result, expected)


def test_custom_callback_accepts_multidimensional_numpy_return(tmp_path) -> None:
    expected = np.array([[1.0, 2.0], [3.0, 4.0]], dtype=np.float64)

    def callback(_inputs: dict[str, libtokamap.MappedValue], _params: dict[str, Any]) -> np.ndarray:
        return expected

    mapper = make_custom_mapper(tmp_path, callback)
    result = mapper.map("example", "magnetics/coil[0]/flux/callback_result", {"shot": 42})

    assert isinstance(result, np.ndarray)
    assert result.shape == (2, 2)
    np.testing.assert_array_equal(result, expected)


@pytest.mark.parametrize(
    "bad_return",
    [
        None,
        [1.0, 2.0],
        1.0,
        np.array(["a", "b"], dtype=object),
        np.array([1 + 2j], dtype=np.complex128),
        np.arange(6, dtype=np.float64)[::2],
    ],
)
def test_custom_callback_rejects_invalid_return_values(tmp_path, bad_return) -> None:
    def callback(_inputs: dict[str, libtokamap.MappedValue], _params: dict[str, Any]) -> object:
        return bad_return

    mapper = make_custom_mapper(tmp_path, callback)

    with pytest.raises(libtokamap.PythonCallbackError):
        mapper.map("example", "magnetics/coil[0]/flux/callback_result", {"shot": 42})
