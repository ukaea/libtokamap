from __future__ import annotations

import gc

import numpy as np

from helpers import make_config, make_mapper


def test_mapped_array_survives_mapper_destruction(tmp_path) -> None:
    mapper = make_mapper(make_config(tmp_path))
    result = mapper.map("example", "magnetics/coil[0]/flux/data", {"shot": 42})
    expected = np.array(result, copy=True)

    del mapper
    gc.collect()

    assert isinstance(result, np.ndarray)
    np.testing.assert_allclose(result, expected)


def test_mapped_scalar_survives_mapper_destruction(tmp_path) -> None:
    mapper = make_mapper(make_config(tmp_path))
    result = mapper.map("example", "magnetics/coil[1]/area", {"shot": 42})
    expected = result.item()

    del mapper
    gc.collect()

    assert isinstance(result, np.ndarray)
    assert result.shape == ()
    assert result.item() == expected


def test_mapped_arrays_are_independent_between_calls(tmp_path) -> None:
    mapper = make_mapper(make_config(tmp_path))
    first = mapper.map("example", "magnetics/coil[0]/flux/data", {"shot": 42})
    second = mapper.map("example", "magnetics/coil[0]/flux/data", {"shot": 42})
    second_expected = np.array(second, copy=True)

    first[0] = 12345.0

    assert second[0] == second_expected[0]
    assert not np.shares_memory(first, second)


def test_mapped_string_survives_mapper_destruction(tmp_path) -> None:
    mapper = make_mapper(make_config(tmp_path))
    result = mapper.map("example", "magnetics/coil[0]/name", {"shot": 42})

    del mapper
    gc.collect()

    assert result == "coil1"
