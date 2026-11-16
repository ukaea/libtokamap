#include "python_utils.hpp"

#include <string>

namespace
{

using libtokamap::python::PyRef;

std::string to_utf8(PyObject* object)
{
    if (object == nullptr) {
        return {};
    }
    PyRef string = PyRef::steal(PyObject_Str(object));
    if (!string) {
        PyErr_Clear();
        return "<unprintable>";
    }
    Py_ssize_t size = 0;
    const char* utf8 = PyUnicode_AsUTF8AndSize(string.get(), &size);
    if (utf8 == nullptr) {
        PyErr_Clear();
        return "<unprintable>";
    }
    return {utf8, static_cast<size_t>(size)};
}

// Location of the innermost traceback frame, i.e. where the exception was raised
std::string traceback_location(PyObject* exception)
{
    PyRef traceback = PyRef::steal(PyException_GetTraceback(exception));
    if (!traceback) {
        return {};
    }
    while (true) {
        PyRef next = PyRef::steal(PyObject_GetAttrString(traceback.get(), "tb_next"));
        if (!next || next.get() == Py_None) {
            break;
        }
        traceback = std::move(next);
    }
    PyRef line = PyRef::steal(PyObject_GetAttrString(traceback.get(), "tb_lineno"));
    PyRef frame = PyRef::steal(PyObject_GetAttrString(traceback.get(), "tb_frame"));
    PyRef code = frame ? PyRef::steal(PyObject_GetAttrString(frame.get(), "f_code")) : PyRef{};
    PyRef file = code ? PyRef::steal(PyObject_GetAttrString(code.get(), "co_filename")) : PyRef{};
    if (!line || !file) {
        PyErr_Clear();
        return {};
    }
    return to_utf8(file.get()) + ":" + to_utf8(line.get());
}

} // namespace

std::string libtokamap::python::fetch_python_error()
{
#if PY_VERSION_HEX >= 0x030C0000
    PyRef exception = PyRef::steal(PyErr_GetRaisedException());
#else
    PyObject* type = nullptr;
    PyObject* value = nullptr;
    PyObject* traceback = nullptr;
    PyErr_Fetch(&type, &value, &traceback);
    PyErr_NormalizeException(&type, &value, &traceback);
    if (value != nullptr && traceback != nullptr) {
        PyException_SetTraceback(value, traceback);
    }
    Py_XDECREF(type);
    Py_XDECREF(traceback);
    PyRef exception = PyRef::steal(value);
#endif
    if (!exception) {
        return "unknown Python error (no exception set)";
    }

    std::string message = type_name(exception.get());
    std::string text = to_utf8(exception.get());
    if (!text.empty()) {
        message += ": " + text;
    }
    std::string location = traceback_location(exception.get());
    if (!location.empty()) {
        message += " (at " + location + ")";
    }
    return message;
}

void libtokamap::python::throw_python_error(const std::string& context)
{
    throw PythonError{context + ": " + fetch_python_error()};
}

bool libtokamap::python::interpreter_usable()
{
#if PY_VERSION_HEX >= 0x030D0000
    return Py_IsInitialized() != 0 && Py_IsFinalizing() == 0;
#else
    return Py_IsInitialized() != 0 && _Py_IsFinalizing() == 0;
#endif
}

std::string libtokamap::python::type_name(PyObject* object)
{
    if (object == nullptr) {
        return "NULL";
    }
    return Py_TYPE(object)->tp_name;
}
