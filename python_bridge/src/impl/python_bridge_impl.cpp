// Python-dependent half of the bridge. Loaded by the shim (python_bridge_shim.cpp) once the Python C API symbols are
// globally visible in the process; this library is deliberately not linked against libpython.

#include "common/python_conversions.hpp"
#include "common/python_utils.hpp"
#include "python_data_source.hpp"
#include "python_runtime.hpp"

#include <memory>
#include <string>

#include "exceptions/exceptions.hpp"
#include "utils/library_loader.hpp"

namespace
{

using libtokamap::python::PyRef;
using libtokamap::python::PythonError;
using libtokamap::python::throw_python_error;

constexpr const char* module_arg = "module";
constexpr const char* class_name_arg = "class_name";

std::unique_ptr<libtokamap::DataSource> create_python_data_source(const libtokamap::DataSourceFactoryArgs& args)
{
    auto module_name = libtokamap::get_arg<std::string>(args, module_arg);
    auto class_name = libtokamap::get_arg<std::string>(args, class_name_arg);
    std::string description = module_name + "." + class_name;

    libtokamap::python::ensure_interpreter();
    libtokamap::python::GilGuard gil;
    try {
        PyRef module = PyRef::steal(PyImport_ImportModule(module_name.c_str()));
        if (!module) {
            throw_python_error("failed to import module '" + module_name + "'");
        }
        PyRef data_source_class = PyRef::steal(PyObject_GetAttrString(module.get(), class_name.c_str()));
        if (!data_source_class) {
            throw_python_error("failed to find '" + class_name + "' in module '" + module_name + "'");
        }
        if (PyCallable_Check(data_source_class.get()) == 0) {
            throw PythonError{"'" + description + "' is not callable"};
        }

        PyRef kwargs = PyRef::steal(PyDict_New());
        if (!kwargs) {
            throw_python_error("failed to create keyword arguments");
        }
        for (const auto& [name, value] : args) {
            if (name == module_arg || name == class_name_arg) {
                continue;
            }
            PyRef item = libtokamap::python::factory_arg_to_python(name, value);
            if (PyDict_SetItemString(kwargs.get(), name.c_str(), item.get()) != 0) {
                throw_python_error("failed to set keyword argument '" + name + "'");
            }
        }

        PyRef no_args = PyRef::steal(PyTuple_New(0));
        if (!no_args) {
            throw_python_error("failed to create argument tuple");
        }
        PyRef instance = PyRef::steal(PyObject_Call(data_source_class.get(), no_args.get(), kwargs.get()));
        if (!instance) {
            throw_python_error("failed to construct instance");
        }

        PyRef get_method = PyRef::steal(PyObject_GetAttrString(instance.get(), "get"));
        if (!get_method || PyCallable_Check(get_method.get()) == 0) {
            PyErr_Clear();
            throw PythonError{"instance has no callable 'get' method"};
        }

        return std::make_unique<libtokamap::python::PythonDataSource>(std::move(instance), description);
    } catch (const PythonError& error) {
        throw libtokamap::ConfigurationError{"Python data source '" + description + "': " + error.what()};
    }
}

} // namespace

LIBTOKAMAP_FACTORY_EXPORT void LibTokaMapPythonBridgeImplLoader(libtokamap::FactoryEntryInterface& entry)
{
    entry.function = create_python_data_source;
}
