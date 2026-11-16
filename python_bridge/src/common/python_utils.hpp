#pragma once

// Small RAII helpers over the CPython C API, shared by the embedded Python bridge and the clibtokamap extension
// module.
//
// Python.h must be included before any standard headers, so this header includes it first.

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdexcept>
#include <string>
#include <utility>

namespace libtokamap::python
{

/**
 * Error raised by the helpers in this namespace when a Python operation fails.
 *
 * The Python error indicator has already been cleared and its text captured in what(). Callers translate this into
 * the LibTokaMap exception that suits their context (configuration vs. callback failure).
 */
class PythonError : public std::runtime_error
{
  public:
    explicit PythonError(const std::string& message) : std::runtime_error(message) {}
};

/**
 * Acquire the GIL for the lifetime of this object. Safe to nest and to use from threads Python has never seen.
 */
class GilGuard
{
  public:
    GilGuard() : m_state{PyGILState_Ensure()} {}
    ~GilGuard() { PyGILState_Release(m_state); }

    GilGuard(const GilGuard&) = delete;
    GilGuard(GilGuard&&) = delete;
    GilGuard& operator=(const GilGuard&) = delete;
    GilGuard& operator=(GilGuard&&) = delete;

  private:
    PyGILState_STATE m_state;
};

/**
 * Owning strong reference to a Python object. Must only be created, moved into, or destroyed while holding the GIL.
 */
class PyRef
{
  public:
    PyRef() = default;
    ~PyRef() { Py_XDECREF(m_object); }

    // Take ownership of a new reference (e.g. the result of most C API calls)
    static PyRef steal(PyObject* object) { return PyRef{object}; }
    // Create a new strong reference to a borrowed object
    static PyRef borrow(PyObject* object)
    {
        Py_XINCREF(object);
        return PyRef{object};
    }

    PyRef(const PyRef&) = delete;
    PyRef& operator=(const PyRef&) = delete;
    PyRef(PyRef&& other) noexcept : m_object{std::exchange(other.m_object, nullptr)} {}
    PyRef& operator=(PyRef&& other) noexcept
    {
        if (this != &other) {
            Py_XDECREF(m_object);
            m_object = std::exchange(other.m_object, nullptr);
        }
        return *this;
    }

    [[nodiscard]] PyObject* get() const { return m_object; }
    [[nodiscard]] PyObject* release() { return std::exchange(m_object, nullptr); }
    explicit operator bool() const { return m_object != nullptr; }

  private:
    explicit PyRef(PyObject* object) : m_object{object} {}

    PyObject* m_object = nullptr;
};

/**
 * Fetch and clear the current Python error, formatted as "ExceptionType: message (at file:line)".
 *
 * Returns a generic message if no error is set. Must be called with the GIL held.
 */
std::string fetch_python_error();

/**
 * Throw a PythonError built from the current Python error, prefixed with context. Must be called with the GIL held.
 */
[[noreturn]] void throw_python_error(const std::string& context);

/**
 * True if the interpreter is initialised and not shutting down, i.e. it is safe to acquire the GIL.
 *
 * Safe to call without holding the GIL.
 */
bool interpreter_usable();

/**
 * Name of the Python type of the given object, for error messages. Must be called with the GIL held.
 */
std::string type_name(PyObject* object);

} // namespace libtokamap::python
