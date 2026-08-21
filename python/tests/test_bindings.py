from __future__ import annotations

import json
from pathlib import Path
from typing import Any

import numpy as np
import pytest

import libtokamap
from helpers import (
    JSONDataSource,
    REPO_ROOT,
    copy_example_mappings,
    dot_product,
    make_config,
    make_mapper,
    write_config,
)


class FailingDataSource(libtokamap.DataSource):
    def get(self, args: dict[str, str]) -> np.ndarray:
        raise ValueError(f"failed for {args['signal']}")


class RecordingDataSource(libtokamap.DataSource):
    def __init__(self) -> None:
        self.calls: list[dict[str, Any]] = []

    def get(self, args: dict[str, Any]) -> np.ndarray:
        self.calls.append(dict(args))
        return np.array([args["shot"]], dtype=np.int64)


class RuntimeAttributeDataSource(libtokamap.DataSource):
    def get(self, args: dict[str, Any]) -> np.ndarray:
        return np.array([args["run_id"]], dtype=np.int64)


class ArrayRecordingDataSource(libtokamap.DataSource):
    def __init__(self) -> None:
        self.calls: list[dict[str, Any]] = []

    def get(self, args: dict[str, Any]) -> np.ndarray:
        self.calls.append(dict(args))
        return np.array([10.0, 20.0, 30.0], dtype=np.float64)


class ConstantRecordingDataSource(libtokamap.DataSource):
    def __init__(self, value: int) -> None:
        self.value = value
        self.calls: list[dict[str, Any]] = []

    def get(self, args: dict[str, Any]) -> np.ndarray:
        self.calls.append(dict(args))
        return np.array([self.value], dtype=np.int64)


def int_codes(_inputs: dict[str, libtokamap.MappedValue], _params: dict[str, Any]) -> np.ndarray:
    return np.array([1, 2, 3], dtype=np.int32)


def failing_custom_function(_inputs: dict[str, libtokamap.MappedValue], _params: dict[str, Any]) -> np.ndarray:
    raise ValueError("custom function failed")


@pytest.fixture
def config_path(tmp_path: Path) -> Path:
    return make_config(tmp_path)


@pytest.fixture
def mapper(config_path: Path) -> libtokamap.Mapper:
    return make_mapper(config_path)


def test_public_exports_and_exception_hierarchy() -> None:
    assert isinstance(libtokamap.__version__, str)
    assert isinstance(libtokamap.LibrarySuffix, str)
    assert issubclass(libtokamap.ConfigurationError, libtokamap.LibTokaMapError)
    assert issubclass(libtokamap.MappingError, libtokamap.LibTokaMapError)
    assert issubclass(libtokamap.MissingMappingError, libtokamap.MappingError)
    assert issubclass(libtokamap.InvalidMappingError, libtokamap.MappingError)
    assert issubclass(libtokamap.DataSourceError, libtokamap.LibTokaMapError)
    assert issubclass(libtokamap.PythonCallbackError, libtokamap.DataSourceError)
    assert issubclass(libtokamap.FileError, libtokamap.LibTokaMapError)
    assert issubclass(libtokamap.JsonError, libtokamap.LibTokaMapError)
    assert issubclass(libtokamap.DataTypeError, libtokamap.LibTokaMapError)
    assert issubclass(libtokamap.PathError, libtokamap.LibTokaMapError)
    assert issubclass(libtokamap.SchemaError, libtokamap.LibTokaMapError)
    assert issubclass(libtokamap.ParameterError, libtokamap.LibTokaMapError)
    assert issubclass(libtokamap.ProcessingError, libtokamap.LibTokaMapError)


def test_invalid_config_extension_raises_configuration_error(tmp_path: Path) -> None:
    config = tmp_path / "config.txt"
    config.write_text("", encoding="utf-8")

    with pytest.raises(libtokamap.ConfigurationError, match="Unsupported configuration file type"):
        libtokamap.Mapper(str(config))


def test_missing_schema_directory_raises_file_error(tmp_path: Path) -> None:
    config = write_config(
        tmp_path / "config.toml",
        REPO_ROOT / "examples/simple_mapper/mappings",
        tmp_path / "missing-schemas",
    )

    with pytest.raises(libtokamap.FileError, match="Schemas directory not found"):
        libtokamap.Mapper(str(config))


def test_invalid_json_config_raises_json_error(tmp_path: Path) -> None:
    config = tmp_path / "config.json"
    config.write_text("{not-json", encoding="utf-8")

    with pytest.raises(libtokamap.JsonError):
        libtokamap.Mapper(str(config))


def test_config_schema_validation_raises_schema_error(tmp_path: Path) -> None:
    config = tmp_path / "config.json"
    config.write_text(
        json.dumps(
            {
                "mapping_directory": 42,
                "schemas_directory": str(REPO_ROOT / "examples/simple_mapper/schemas"),
            }
        ),
        encoding="utf-8",
    )

    with pytest.raises(libtokamap.SchemaError):
        libtokamap.Mapper(str(config))


def test_maps_example_data_through_python_bindings(mapper: libtokamap.Mapper) -> None:
    attributes = {"shot": 42}

    n_coils = mapper.map("example", "magnetics/coil", attributes)
    assert n_coils.shape == ()
    assert n_coils.dtype == np.dtype("uint64")
    assert n_coils.item() == 3

    name = mapper.map("example", "magnetics/coil[0]/name", attributes)
    assert name == "coil1"

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


def test_python_data_source_receives_runtime_attributes(
    config_path: Path, tmp_path: Path
) -> None:
    mapping_dir = copy_example_mappings(tmp_path)
    globals_path = mapping_dir / "example_v1/globals.json"
    globals_json = json.loads(globals_path.read_text(encoding="utf-8"))
    globals_json["DEG2RAD"] = 0.0174532925199
    globals_path.write_text(json.dumps(globals_json), encoding="utf-8")

    mappings_path = mapping_dir / "example_v1/magnetics/40/mappings.json"
    mappings = json.loads(mappings_path.read_text(encoding="utf-8"))
    mappings["runtime_attrs"] = {
        "map_type": "DATA_SOURCE",
        "data_source": "JSON",
        "args": {"signal": "runtime_attrs"},
    }
    mappings["explicit_args_override_runtime_attrs"] = {
        "map_type": "DATA_SOURCE",
        "data_source": "JSON",
        "args": {"signal": "runtime_attrs", "shot": 7},
    }
    mappings_path.write_text(json.dumps(mappings), encoding="utf-8")

    write_config(config_path, mapping_dir, REPO_ROOT / "examples/simple_mapper/schemas")
    data_source = RecordingDataSource()
    mapper = libtokamap.Mapper(str(config_path))
    mapper.register_python_data_source("JSON", data_source)
    mapper.register_custom_function("custom", "dot_product", dot_product)

    runtime_result = mapper.map("example", "magnetics/runtime_attrs", {"shot": 42})
    override_result = mapper.map(
        "example", "magnetics/explicit_args_override_runtime_attrs", {"shot": 42}
    )

    np.testing.assert_array_equal(runtime_result, np.array([42], dtype=np.int64))
    np.testing.assert_array_equal(override_result, np.array([7], dtype=np.int64))
    assert data_source.calls[0]["shot"] == 42
    assert data_source.calls[0]["signal"] == "runtime_attrs"
    assert "DEG2RAD" not in data_source.calls[0]
    assert data_source.calls[1]["shot"] == 7


@pytest.mark.cache_regression
@pytest.mark.skip(reason="skipping until caching is reworked")
def test_cache_distinguishes_indices_and_reuses_raw_data_source_fetch(
    config_path: Path, tmp_path: Path
) -> None:
    mapping_dir = copy_example_mappings(tmp_path)
    mappings_path = mapping_dir / "example_v1/magnetics/40/mappings.json"
    mappings = json.loads(mappings_path.read_text(encoding="utf-8"))
    mappings["cache_probe[#]"] = {
        "map_type": "DATA_SOURCE",
        "data_source": "JSON",
        "args": {"signal": "cache_probe_indexed"},
        "slice": "[{{ #0 }}]",
    }
    mappings_path.write_text(json.dumps(mappings), encoding="utf-8")

    config_path.write_text(
        "\n".join(
            [
                f'mapping_directory = "{mapping_dir.as_posix()}"',
                f'schemas_directory = "{(REPO_ROOT / "examples/simple_mapper/schemas").as_posix()}"',
                "cache_enabled = true",
            ]
        ),
        encoding="utf-8",
    )

    data_source = ArrayRecordingDataSource()
    mapper = libtokamap.Mapper(str(config_path))
    mapper.register_python_data_source("JSON", data_source)
    mapper.register_custom_function("custom", "dot_product", dot_product)

    first = mapper.map("example", "magnetics/cache_probe[0]", {"shot": 42})
    second = mapper.map("example", "magnetics/cache_probe[1]", {"shot": 42})

    assert first.item() == pytest.approx(10.0)
    assert second.item() == pytest.approx(20.0)
    assert len(data_source.calls) == 1


@pytest.mark.cache_regression
@pytest.mark.skip(reason="skipping until caching is reworked")
def test_repeated_identical_data_source_request_reuses_raw_fetch(
    config_path: Path, tmp_path: Path
) -> None:
    mapping_dir = copy_example_mappings(tmp_path)
    mappings_path = mapping_dir / "example_v1/magnetics/40/mappings.json"
    mappings = json.loads(mappings_path.read_text(encoding="utf-8"))
    mappings["cache_probe"] = {
        "map_type": "DATA_SOURCE",
        "data_source": "JSON",
        "args": {"signal": "cache_probe_repeated"},
    }
    mappings_path.write_text(json.dumps(mappings), encoding="utf-8")

    config_path.write_text(
        "\n".join(
            [
                f'mapping_directory = "{mapping_dir.as_posix()}"',
                f'schemas_directory = "{(REPO_ROOT / "examples/simple_mapper/schemas").as_posix()}"',
                "cache_enabled = true",
            ]
        ),
        encoding="utf-8",
    )

    data_source = ArrayRecordingDataSource()
    mapper = libtokamap.Mapper(str(config_path))
    mapper.register_python_data_source("JSON", data_source)
    mapper.register_custom_function("custom", "dot_product", dot_product)

    first = mapper.map("example", "magnetics/cache_probe", {"shot": 42})
    second = mapper.map("example", "magnetics/cache_probe", {"shot": 42})

    np.testing.assert_array_equal(first, np.array([10.0, 20.0, 30.0]))
    np.testing.assert_array_equal(second, np.array([10.0, 20.0, 30.0]))
    assert len(data_source.calls) == 1


@pytest.mark.cache_regression
@pytest.mark.skip(reason="skipping until caching is reworked")
def test_sibling_mappings_with_same_data_source_args_reuse_raw_fetch(
    config_path: Path, tmp_path: Path
) -> None:
    mapping_dir = copy_example_mappings(tmp_path)
    mappings_path = mapping_dir / "example_v1/magnetics/40/mappings.json"
    mappings = json.loads(mappings_path.read_text(encoding="utf-8"))
    mappings["cache_probe_raw"] = {
        "map_type": "DATA_SOURCE",
        "data_source": "JSON",
        "args": {"signal": "cache_probe_sibling"},
    }
    mappings["cache_probe_scaled"] = {
        "map_type": "DATA_SOURCE",
        "data_source": "JSON",
        "args": {"signal": "cache_probe_sibling"},
        "scale": 2.0,
    }
    mappings_path.write_text(json.dumps(mappings), encoding="utf-8")

    config_path.write_text(
        "\n".join(
            [
                f'mapping_directory = "{mapping_dir.as_posix()}"',
                f'schemas_directory = "{(REPO_ROOT / "examples/simple_mapper/schemas").as_posix()}"',
                "cache_enabled = true",
            ]
        ),
        encoding="utf-8",
    )

    data_source = ArrayRecordingDataSource()
    mapper = libtokamap.Mapper(str(config_path))
    mapper.register_python_data_source("JSON", data_source)
    mapper.register_custom_function("custom", "dot_product", dot_product)

    raw = mapper.map("example", "magnetics/cache_probe_raw", {"shot": 42})
    scaled = mapper.map("example", "magnetics/cache_probe_scaled", {"shot": 42})

    np.testing.assert_array_equal(raw, np.array([10.0, 20.0, 30.0]))
    np.testing.assert_array_equal(scaled, np.array([20.0, 40.0, 60.0]))
    assert len(data_source.calls) == 1


def test_cache_disabled_repeated_data_source_request_fetches_each_time(
    config_path: Path, tmp_path: Path
) -> None:
    mapping_dir = copy_example_mappings(tmp_path)
    mappings_path = mapping_dir / "example_v1/magnetics/40/mappings.json"
    mappings = json.loads(mappings_path.read_text(encoding="utf-8"))
    mappings["uncached_probe"] = {
        "map_type": "DATA_SOURCE",
        "data_source": "JSON",
        "args": {"signal": "cache_probe_disabled"},
    }
    mappings_path.write_text(json.dumps(mappings), encoding="utf-8")

    config_path.write_text(
        "\n".join(
            [
                f'mapping_directory = "{mapping_dir.as_posix()}"',
                f'schemas_directory = "{(REPO_ROOT / "examples/simple_mapper/schemas").as_posix()}"',
                "cache_enabled = false",
            ]
        ),
        encoding="utf-8",
    )

    data_source = ArrayRecordingDataSource()
    mapper = libtokamap.Mapper(str(config_path))
    mapper.register_python_data_source("JSON", data_source)
    mapper.register_custom_function("custom", "dot_product", dot_product)

    mapper.map("example", "magnetics/uncached_probe", {"shot": 42})
    mapper.map("example", "magnetics/uncached_probe", {"shot": 42})

    assert len(data_source.calls) == 2


@pytest.mark.cache_regression
@pytest.mark.skip(reason="skipping until caching is reworked")
def test_data_source_cache_does_not_leak_between_mapper_instances(
    config_path: Path, tmp_path: Path
) -> None:
    mapping_dir = copy_example_mappings(tmp_path)
    mappings_path = mapping_dir / "example_v1/magnetics/40/mappings.json"
    mappings = json.loads(mappings_path.read_text(encoding="utf-8"))
    mappings["pollution_probe"] = {
        "map_type": "DATA_SOURCE",
        "data_source": "JSON",
        "args": {"signal": "cache_probe_pollution"},
    }
    mappings_path.write_text(json.dumps(mappings), encoding="utf-8")

    config_path.write_text(
        "\n".join(
            [
                f'mapping_directory = "{mapping_dir.as_posix()}"',
                f'schemas_directory = "{(REPO_ROOT / "examples/simple_mapper/schemas").as_posix()}"',
                "cache_enabled = true",
            ]
        ),
        encoding="utf-8",
    )

    first_source = ConstantRecordingDataSource(1)
    first_mapper = libtokamap.Mapper(str(config_path))
    first_mapper.register_python_data_source("JSON", first_source)
    first_mapper.register_custom_function("custom", "dot_product", dot_product)
    np.testing.assert_array_equal(
        first_mapper.map("example", "magnetics/pollution_probe", {"shot": 42}),
        np.array([1], dtype=np.int64),
    )

    second_source = ConstantRecordingDataSource(2)
    second_mapper = libtokamap.Mapper(str(config_path))
    second_mapper.register_python_data_source("JSON", second_source)
    second_mapper.register_custom_function("custom", "dot_product", dot_product)
    np.testing.assert_array_equal(
        second_mapper.map("example", "magnetics/pollution_probe", {"shot": 42}),
        np.array([2], dtype=np.int64),
    )

    third_source = ConstantRecordingDataSource(3)
    third_mapper = libtokamap.Mapper(str(config_path))
    third_mapper.register_python_data_source("JSON", third_source)
    third_mapper.register_custom_function("custom", "dot_product", dot_product)
    np.testing.assert_array_equal(
        third_mapper.map("example", "magnetics/pollution_probe", {"shot": 42}),
        np.array([3], dtype=np.int64),
    )
    assert len(third_source.calls) == 1


@pytest.mark.cache_regression
@pytest.mark.skip(reason="skipping until caching is reworked")
def test_mapping_cache_key_includes_runtime_attributes(
    config_path: Path, tmp_path: Path
) -> None:
    mapping_dir = copy_example_mappings(tmp_path)
    mappings_path = mapping_dir / "example_v1/magnetics/40/mappings.json"
    mappings = json.loads(mappings_path.read_text(encoding="utf-8"))
    mappings["_runtime_attr"] = {
        "map_type": "DATA_SOURCE",
        "data_source": "JSON",
        "args": {"signal": "runtime_attr_key", "run_id": "{{ run_id }}"},
    }
    mappings["runtime_attr_ref"] = {
        "map_type": "EXPR",
        "expr": "x",
        "parameters": {"x": "_runtime_attr"},
    }
    mappings_path.write_text(json.dumps(mappings), encoding="utf-8")

    config_path.write_text(
        "\n".join(
            [
                f'mapping_directory = "{mapping_dir.as_posix()}"',
                f'schemas_directory = "{(REPO_ROOT / "examples/simple_mapper/schemas").as_posix()}"',
                "cache_enabled = true",
            ]
        ),
        encoding="utf-8",
    )

    data_source = RuntimeAttributeDataSource()
    mapper = libtokamap.Mapper(str(config_path))
    mapper.register_python_data_source("JSON", data_source)
    mapper.register_custom_function("custom", "dot_product", dot_product)

    first = mapper.map("example", "magnetics/_runtime_attr", {"shot": 42, "run_id": 1})
    second = mapper.map("example", "magnetics/_runtime_attr", {"shot": 42, "run_id": 2})

    np.testing.assert_array_equal(first, np.array([1], dtype=np.int64))
    np.testing.assert_array_equal(second, np.array([2], dtype=np.int64))


def test_custom_function_errors_are_wrapped(config_path: Path, tmp_path: Path) -> None:
    mapping_dir = copy_example_mappings(tmp_path)
    mappings_path = mapping_dir / "example_v1/magnetics/40/mappings.json"
    mappings = json.loads(mappings_path.read_text(encoding="utf-8"))
    mappings["coil[#]/flux/failing_custom"] = {
        "map_type": "CUSTOM",
        "library": "custom",
        "function": "failing_custom",
        "inputs": {"lhs": "coil[#]/flux/time"},
        "parameters": {},
    }
    mappings_path.write_text(json.dumps(mappings), encoding="utf-8")

    write_config(config_path, mapping_dir, REPO_ROOT / "examples/simple_mapper/schemas")
    mapper = libtokamap.Mapper(str(config_path))
    mapper.register_python_data_source("JSON", JSONDataSource(REPO_ROOT / "examples/simple_mapper/data"))
    mapper.register_custom_function("custom", "dot_product", dot_product)
    mapper.register_custom_function("custom", "failing_custom", failing_custom_function)

    with pytest.raises(libtokamap.PythonCallbackError, match="custom function failed"):
        mapper.map("example", "magnetics/coil[0]/flux/failing_custom", {"shot": 42})


def test_custom_function_accepts_integer_numpy_return(config_path: Path, tmp_path: Path) -> None:
    mapping_dir = copy_example_mappings(tmp_path)
    mappings_path = mapping_dir / "example_v1/magnetics/40/mappings.json"
    mappings = json.loads(mappings_path.read_text(encoding="utf-8"))
    mappings["coil[#]/flux/int_codes"] = {
        "map_type": "CUSTOM",
        "library": "custom",
        "function": "int_codes",
        "inputs": {},
        "parameters": {},
    }
    mappings_path.write_text(json.dumps(mappings), encoding="utf-8")

    write_config(config_path, mapping_dir, REPO_ROOT / "examples/simple_mapper/schemas")
    mapper = libtokamap.Mapper(str(config_path))
    mapper.register_python_data_source("JSON", JSONDataSource(REPO_ROOT / "examples/simple_mapper/data"))
    mapper.register_custom_function("custom", "dot_product", dot_product)
    mapper.register_custom_function("custom", "int_codes", int_codes)

    result = mapper.map("example", "magnetics/coil[0]/flux/int_codes", {"shot": 42})

    assert isinstance(result, np.ndarray)
    assert result.dtype == np.dtype("int32")
    np.testing.assert_array_equal(result, np.array([1, 2, 3], dtype=np.int32))


def test_unregistered_data_source_raises_data_source_error(config_path: Path) -> None:
    mapper = libtokamap.Mapper(str(config_path))
    mapper.register_custom_function("custom", "dot_product", dot_product)

    with pytest.raises(libtokamap.DataSourceError, match="not registered"):
        mapper.map("example", "magnetics/coil", {"shot": 42})


def test_empty_mapping_path_raises_path_error(mapper: libtokamap.Mapper) -> None:
    with pytest.raises(libtokamap.PathError, match="Mapping path could not be split"):
        mapper.map("example", "", {"shot": 42})


def test_missing_partition_attribute_raises_parameter_error(mapper: libtokamap.Mapper) -> None:
    with pytest.raises(libtokamap.ParameterError, match="required attribute 'shot' not provided"):
        mapper.map("example", "magnetics/coil", {})


def test_invalid_slice_raises_processing_error(config_path: Path, tmp_path: Path) -> None:
    mapping_dir = copy_example_mappings(tmp_path)
    mappings_path = mapping_dir / "example_v1/magnetics/40/mappings.json"
    mappings = json.loads(mappings_path.read_text(encoding="utf-8"))
    mappings["bad_slice"] = {
        "map_type": "DATA_SOURCE",
        "data_source": "JSON",
        "args": {"signal": "coils/0/flux/data"},
        "slice": "[::0]",
    }
    mappings_path.write_text(json.dumps(mappings), encoding="utf-8")

    write_config(config_path, mapping_dir, REPO_ROOT / "examples/simple_mapper/schemas")
    mapper = libtokamap.Mapper(str(config_path))
    mapper.register_python_data_source("JSON", JSONDataSource(REPO_ROOT / "examples/simple_mapper/data"))
    mapper.register_custom_function("custom", "dot_product", dot_product)

    with pytest.raises(libtokamap.ProcessingError, match="stride of 0"):
        mapper.map("example", "magnetics/bad_slice", {"shot": 42})


def test_missing_mapping_raises_missing_mapping_error(mapper: libtokamap.Mapper) -> None:
    with pytest.raises(libtokamap.MissingMappingError):
        mapper.map("example", "magnetics/missing", {"shot": 42})
