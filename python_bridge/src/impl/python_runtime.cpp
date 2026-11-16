#include "python_runtime.hpp"

#include "common/python_utils.hpp"

#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <string>

#include "exceptions/exceptions.hpp"

namespace
{

constexpr const char* python_executable_env = "LIBTOKAMAP_PYTHON_EXECUTABLE";
constexpr const char* virtual_env_env = "VIRTUAL_ENV";

std::mutex g_runtime_mutex;
bool g_runtime_ready = false;

std::string configured_executable()
{
    if (const char* executable = std::getenv(python_executable_env); executable != nullptr && *executable != '\0') {
        return executable;
    }
    if (const char* venv = std::getenv(virtual_env_env); venv != nullptr && *venv != '\0') {
        auto executable = std::filesystem::path{venv} / "bin" / "python";
        if (std::filesystem::exists(executable)) {
            return executable.string();
        }
    }
    return {};
}

void check_status(const PyStatus& status, const std::string& context)
{
    if (PyStatus_Exception(status) != 0) {
        std::string message = "failed to initialise embedded Python (" + context + ")";
        if (status.func != nullptr) {
            message += std::string{" in "} + status.func;
        }
        if (status.err_msg != nullptr) {
            message += std::string{": "} + status.err_msg;
        }
        throw libtokamap::ConfigurationError{message};
    }
}

void initialise_interpreter()
{
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    // The host application owns signal handling and the command line
    config.install_signal_handlers = 0;
    config.parse_argv = 0;

    try {
        std::string executable = configured_executable();
        if (!executable.empty()) {
            // getpath derives sys.prefix, and detects a virtual environment's pyvenv.cfg, from the program name
            check_status(PyConfig_SetBytesString(&config, &config.program_name, executable.c_str()),
                         "setting program name to '" + executable + "'");
        }
        check_status(Py_InitializeFromConfig(&config), "Py_InitializeFromConfig");
    } catch (...) {
        PyConfig_Clear(&config);
        throw;
    }
    PyConfig_Clear(&config);

    // Initialisation leaves this thread holding the GIL; release it so any thread can take it via PyGILState_Ensure.
    // The returned main thread state is intentionally kept for the life of the process.
    PyEval_SaveThread();
}

} // namespace

void libtokamap::python::ensure_interpreter()
{
    std::lock_guard lock{g_runtime_mutex};
    if (g_runtime_ready) {
        if (!interpreter_usable()) {
            throw libtokamap::ConfigurationError{"Python interpreter has been finalised"};
        }
        return;
    }
    if (Py_IsInitialized() == 0) {
        initialise_interpreter();
    }
    g_runtime_ready = true;
}
