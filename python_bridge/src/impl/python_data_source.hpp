#pragma once

#include "common/python_utils.hpp"

#include <string>

#include "map_types/data_source_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/ram_cache.hpp"
#include "utils/typed_data_array.hpp"

namespace libtokamap::python
{

/**
 * LibTokaMap data source backed by a Python object with a `get(args: dict) -> array | str` method.
 *
 * Every call acquires the GIL, so the data source may be used from any thread; calls into Python are serialised.
 * Python failures are raised as libtokamap::PythonCallbackError.
 */
class PythonDataSource final : public libtokamap::DataSource
{
  public:
    // Must be called with the GIL held
    PythonDataSource(PyRef instance, std::string description);
    ~PythonDataSource() override;

    PythonDataSource(const PythonDataSource&) = delete;
    PythonDataSource(PythonDataSource&&) = delete;
    PythonDataSource& operator=(const PythonDataSource&) = delete;
    PythonDataSource& operator=(PythonDataSource&&) = delete;

    TypedDataArray get(const DataSourceArgs& map_args, const MapArguments& arguments, RamCache* ram_cache) override;

  private:
    PyObject* m_instance;
    PyObject* m_get_name;
    std::string m_description;
};

} // namespace libtokamap::python
