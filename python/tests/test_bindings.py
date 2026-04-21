from __future__ import annotations

import json
from pathlib import Path
from typing import Any

import numpy as np
import pytest

import libtokamap


REPO_ROOT = Path(__file__).resolve().parents[2]


class JSONDataSource(libtokamap.DataSource):
    def __init__(self, data_root: Path):
        self.data_root = data_root

    def get(self, args: dict[str, str]) -> np.ndarray:
        with (self.data_root / args["file_name"]).open() as handle:
            data = json.load(handle)

        for token in args["signal"].split("/"):
            try:
                data = data[int(token)]
            except ValueError:
                data = data[token]

        return np.array(data)


class FailingDataSource(libtokamap.DataSource):
    def get(self, args: dict[str, str]) -> np.ndarray:
        raise ValueError(f"failed for {args['signal']}")


def dot_product(inputs: dict[str, np.ndarray], _params: dict[str, Any]) -> np.ndarray:
    return np.array(np.dot(inputs["lhs"], inputs["rhs"]))


def decode_s1_array(value: np.ndarray) -> str:
    assert value.dtype == np.dtype("S1")
    return value.tobytes().decode()


@pytest.fixture
def config_path(tmp_path: Path) -> Path:
    config = tmp_path / "config.toml"
    config.write_text(
        "\n".join(
            [
                f'mapping_directory = "{REPO_ROOT / "examples/simple_mapper/mappings"}"',
                f'schemas_directory = "{REPO_ROOT / "examples/simple_mapper/schemas"}"',
                "cache_enabled = false",
            ]
        ),
        encoding="utf-8",
    )
    return config


@pytest.fixture
def mapper(config_path: Path) -> libtokamap.Mapper:
    mapper = libtokamap.Mapper(str(config_path))
    mapper.register_python_data_source("JSON", JSONDataSource(REPO_ROOT / "examples/simple_mapper/data"))
    mapper.register_custom_function("custom", "dot_product", dot_product)
    return mapper


def test_public_exports_and_exception_hierarchy() -> None:
    assert isinstance(libtokamap.__version__, str)
    assert isinstance(libtokamap.LibrarySuffix, str)
    assert issubclass(libtokamap.ConfigurationError, libtokamap.LibTokaMapError)
    assert issubclass(libtokamap.MissingMappingError, libtokamap.MappingError)
    assert issubclass(libtokamap.DataSourceError, libtokamap.LibTokaMapError)
    assert issubclass(libtokamap.PythonCallbackError, libtokamap.DataSourceError)


def test_maps_example_data_through_python_bindings(mapper: libtokamap.Mapper) -> None:
    attributes = {"shot": 42}

    n_coils = mapper.map("example", "magnetics/coil", attributes)
    assert n_coils.shape == ()
    assert n_coils.dtype == np.dtype("uint64")
    assert n_coils.item() == 3

    name = mapper.map("example", "magnetics/coil[0]/name", attributes)
    assert decode_s1_array(name) == "coil1"

    area = mapper.map("example", "magnetics/coil[1]/area", attributes)
    assert area.shape == ()
    assert area.dtype == np.dtype("float64")
    assert area.item() == pytest.approx(np.pi * 0.2**2)

    scaled = mapper.map("example", "magnetics/coil[0]/flux/data_scaled", attributes)
    raw = mapper.map("example", "magnetics/coil[0]/flux/data", attributes)
    assert scaled.shape == (100,)
    np.testing.assert_allclose(scaled, raw * 2.0)

    product = mapper.map("example", "magnetics/coil[0]/flux/dot_product", attributes)
    time = mapper.map("example", "magnetics/coil[0]/flux/time", attributes)
    assert product.shape == ()
    assert product.item() == pytest.approx(np.dot(time, raw))


def test_register_python_data_source_rejects_non_datasource(config_path: Path) -> None:
    mapper = libtokamap.Mapper(str(config_path))

    with pytest.raises(libtokamap.LibTokaMapError, match="does not inherit from DataSource"):
        mapper.register_python_data_source("JSON", object())  # type: ignore[arg-type]


def test_python_data_source_errors_are_wrapped(config_path: Path) -> None:
    mapper = libtokamap.Mapper(str(config_path))
    mapper.register_python_data_source("JSON", FailingDataSource())
    mapper.register_custom_function("custom", "dot_product", dot_product)

    with pytest.raises(libtokamap.PythonCallbackError, match="failed for coils"):
        mapper.map("example", "magnetics/coil", {"shot": 42})


def test_missing_mapping_raises_mapping_error(mapper: libtokamap.Mapper) -> None:
    with pytest.raises(libtokamap.MappingError):
        mapper.map("example", "magnetics/missing", {"shot": 42})
