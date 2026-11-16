#include "python_data_source.hpp"

#include "common/python_conversions.hpp"
#include "common/python_utils.hpp"

#include <string>
#include <utility>

#include "exceptions/exceptions.hpp"

libtokamap::python::PythonDataSource::PythonDataSource(PyRef instance, std::string description)
    : m_instance{instance.release()}, m_get_name{PyUnicode_InternFromString("get")},
      m_description{std::move(description)}
{
    if (m_get_name == nullptr) {
        Py_DECREF(m_instance);
        throw_python_error("failed to create method name");
    }
}

libtokamap::python::PythonDataSource::~PythonDataSource()
{
    // If the interpreter is already shutting down (e.g. a Python host exiting before the C++ objects are destroyed)
    // touching Python state is unsafe, so the references are deliberately leaked
    if (!interpreter_usable()) {
        return;
    }
    GilGuard gil;

    // Give the Python object a chance to release resources, since atexit handlers never run in a C++ host
    PyRef close = PyRef::steal(PyObject_GetAttrString(m_instance, "close"));
    if (!close) {
        PyErr_Clear();
    } else if (PyCallable_Check(close.get()) != 0) {
        PyRef result = PyRef::steal(PyObject_CallNoArgs(close.get()));
        if (!result) {
            PyErr_WriteUnraisable(close.get());
        }
    }

    Py_DECREF(m_get_name);
    Py_DECREF(m_instance);
}

libtokamap::TypedDataArray libtokamap::python::PythonDataSource::get(const DataSourceArgs& map_args,
                                                                     const MapArguments& /*arguments*/,
                                                                     RamCache* /*ram_cache*/)
{
    if (!interpreter_usable()) {
        throw PythonCallbackError{m_description + ".get(): Python interpreter is not available"};
    }
    GilGuard gil;
    try {
        PyRef args = args_to_python(map_args);
        PyRef result = PyRef::steal(PyObject_CallMethodOneArg(m_instance, m_get_name, args.get()));
        if (!result) {
            throw PythonError{fetch_python_error()};
        }
        return python_to_typed_data_array(result.get(), ObjectArrays::Placeholder);
    } catch (const PythonError& error) {
        throw PythonCallbackError{m_description + ".get(): " + error.what()};
    }
}
