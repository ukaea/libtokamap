from __future__ import annotations

import json
import shutil
from pathlib import Path
from typing import Any

import numpy as np

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


def dot_product(inputs: dict[str, libtokamap.MappedValue], _params: dict[str, Any]) -> np.ndarray:
    assert isinstance(inputs["lhs"], np.ndarray)
    assert isinstance(inputs["rhs"], np.ndarray)
    return np.array(np.dot(inputs["lhs"], inputs["rhs"]))


def write_config(config_path: Path, mapping_directory: Path, schemas_directory: Path) -> Path:
    config_path.write_text(
        "\n".join(
            [
                f'mapping_directory = "{mapping_directory.as_posix()}"',
                f'schemas_directory = "{schemas_directory.as_posix()}"',
                "cache_enabled = false",
            ]
        ),
        encoding="utf-8",
    )
    return config_path


def copy_example_mappings(tmp_path: Path) -> Path:
    mapping_dir = tmp_path / "mappings"
    shutil.copytree(REPO_ROOT / "examples/simple_mapper/mappings", mapping_dir)
    return mapping_dir


def make_config(tmp_path: Path, mapping_directory: Path | None = None) -> Path:
    return write_config(
        tmp_path / "config.toml",
        mapping_directory or REPO_ROOT / "examples/simple_mapper/mappings",
        REPO_ROOT / "examples/simple_mapper/schemas",
    )


def make_mapper(config_path: Path) -> libtokamap.Mapper:
    mapper = libtokamap.Mapper(str(config_path))
    mapper.register_python_data_source("JSON", JSONDataSource(REPO_ROOT / "examples/simple_mapper/data"))
    mapper.register_custom_function("custom", "dot_product", dot_product)
    return mapper
