#pragma once

#include "python_utils.hpp"

#include <any>
#include <nlohmann/json.hpp>
#include <string>

#include "utils/typed_data_array.hpp"

namespace libtokamap::python
{

/**
 * Convert a JSON value to the equivalent Python object (None, bool, int, float, str, list or dict).
 *
 * Throws PythonError on failure. Must be called with the GIL held.
 */
PyRef json_to_python(const nlohmann::json& value);

/**
 * Convert a data source factory argument to a Python object.
 *
 * Supports the types MappingHandler stores for configured factory arguments: double, int, bool, std::string and
 * nlohmann::json. Throws PythonError for anything else. Must be called with the GIL held.
 */
PyRef factory_arg_to_python(const std::string& name, const std::any& value);

/**
 * Convert a value returned by a Python callback to a TypedDataArray. The data is always copied.
 *
 * Accepted values:
 *  - str, returned as a string array;
 *  - any object exporting the buffer protocol (NumPy arrays and scalars, memoryview, array.array, bytes) with a
 *    boolean, integer, float32 or float64 element type, in any byte order, contiguity or writeability;
 *  - single-element NumPy unicode ('U') or bytes ('S') arrays, returned as a string array.
 *
 * Anything else, including object arrays and multi-element string arrays, raises PythonError. Must be called with
 * the GIL held.
 */
TypedDataArray python_to_typed_data_array(PyObject* object);

} // namespace libtokamap::python
