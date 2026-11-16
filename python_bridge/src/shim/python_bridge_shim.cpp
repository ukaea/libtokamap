// Python-free entry point of the bridge, loaded by LibTokaMap's data source factory loader.
//
// Python extension modules (e.g. NumPy) are not linked against libpython: they expect the Python C API symbols to
// already be globally visible in the process. The factory loader opens this library with local visibility, so if it
// linked libpython directly those symbols would stay hidden. Linking libpython would also load a second copy of
// Python into hosts whose python executable has libpython built in (e.g. Debian, conda).
//
// So this shim:
//   1. uses the Python already in the process if there is one (a Python host), otherwise loads libpython with
//      global visibility;
//   2. checks it is the Python version the bridge was built for;
//   3. loads the implementation library, which uses the now-visible Python symbols, and forwards to its entry point.

#include <dlfcn.h>

#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "utils/library_loader.hpp"

#ifndef LIBTOKAMAP_PYTHON_VERSION
#error "LIBTOKAMAP_PYTHON_VERSION must be defined as the \"major.minor\" Python version the bridge is built for"
#endif
#ifndef LIBTOKAMAP_DEFAULT_PYTHON_LIBRARY
#define LIBTOKAMAP_DEFAULT_PYTHON_LIBRARY ""
#endif
#ifndef LIBTOKAMAP_PYTHON_BRIDGE_IMPL_NAME
#error "LIBTOKAMAP_PYTHON_BRIDGE_IMPL_NAME must be defined as the file name of the bridge implementation library"
#endif

namespace
{

using ImplEntryFunction = void (*)(libtokamap::FactoryEntryInterface&);
using PythonVersionFunction = const char* (*)();

constexpr const char* python_library_env = "LIBTOKAMAP_PYTHON_LIBRARY";
constexpr const char* impl_library_env = "LIBTOKAMAP_PYTHON_BRIDGE_IMPL";
constexpr const char* impl_entry_symbol = "LibTokaMapPythonBridgeImplLoader";

std::mutex g_load_mutex;
ImplEntryFunction g_impl_entry = nullptr;

// Any object in this library, used to find the library's own path
const char g_address_anchor = 0;

std::string last_dl_error()
{
    const char* error = dlerror();
    return error != nullptr ? error : "unknown error";
}

std::filesystem::path this_library_path()
{
    Dl_info info{};
    if (dladdr(&g_address_anchor, &info) == 0 || info.dli_fname == nullptr) {
        throw libtokamap::ConfigurationError{"Python bridge could not determine its own location"};
    }
    return std::filesystem::path{info.dli_fname};
}

bool python_symbols_visible() { return dlsym(RTLD_DEFAULT, "Py_IsInitialized") != nullptr; }

void load_python_library()
{
    std::vector<std::string> candidates;
    if (const char* path = std::getenv(python_library_env); path != nullptr && *path != '\0') {
        candidates.emplace_back(path);
    }
    if (!std::string_view{LIBTOKAMAP_DEFAULT_PYTHON_LIBRARY}.empty()) {
        candidates.emplace_back(LIBTOKAMAP_DEFAULT_PYTHON_LIBRARY);
    }
    if (candidates.empty()) {
        throw libtokamap::ConfigurationError{std::string{"Python bridge: no Python "} + LIBTOKAMAP_PYTHON_VERSION +
                                             " shared library is known; set " + python_library_env +
                                             " to the path of libpython"};
    }

    std::string errors;
    for (const auto& candidate : candidates) {
        if (dlopen(candidate.c_str(), RTLD_NOW | RTLD_GLOBAL) != nullptr) {
            return;
        }
        errors += "\n  " + candidate + ": " + last_dl_error();
    }
    throw libtokamap::ConfigurationError{std::string{"Python bridge: failed to load the Python "} +
                                         LIBTOKAMAP_PYTHON_VERSION + " shared library (set " + python_library_env +
                                         " to override):" + errors};
}

void check_python_version()
{
    auto* version_function = reinterpret_cast<PythonVersionFunction>(dlsym(RTLD_DEFAULT, "Py_GetVersion"));
    if (version_function == nullptr) {
        throw libtokamap::ConfigurationError{"Python bridge: Py_GetVersion not found in the loaded Python"};
    }
    // Py_GetVersion() returns e.g. "3.14.6 (main, ...)": compare the "major.minor" prefix
    std::string version = version_function();
    std::string expected = LIBTOKAMAP_PYTHON_VERSION;
    bool matches = version.starts_with(expected) && version.size() > expected.size() && version[expected.size()] == '.';
    if (!matches) {
        throw libtokamap::ConfigurationError{"Python bridge was built for Python " + expected +
                                             " but the process has Python " + version.substr(0, version.find(' '))};
    }
}

ImplEntryFunction load_implementation()
{
    std::filesystem::path self = this_library_path();

    // Make sure neither library can be unloaded: data sources created by the bridge outlive any one MappingHandler
    dlopen(self.c_str(), RTLD_LAZY | RTLD_NOLOAD | RTLD_NODELETE);

    std::filesystem::path impl_path = self.parent_path() / LIBTOKAMAP_PYTHON_BRIDGE_IMPL_NAME;
    if (const char* path = std::getenv(impl_library_env); path != nullptr && *path != '\0') {
        impl_path = path;
    }

    void* handle = dlopen(impl_path.c_str(), RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE);
    if (handle == nullptr) {
        throw libtokamap::ConfigurationError{"Python bridge: failed to load implementation library '" +
                                             impl_path.string() + "': " + last_dl_error()};
    }
    void* entry = dlsym(handle, impl_entry_symbol);
    if (entry == nullptr) {
        throw libtokamap::ConfigurationError{"Python bridge: entry point '" + std::string{impl_entry_symbol} +
                                             "' not found in '" + impl_path.string() + "'"};
    }
    return reinterpret_cast<ImplEntryFunction>(entry);
}

} // namespace

LIBTOKAMAP_EXPORT_FACTORY_ABI_VERSION()

LIBTOKAMAP_FACTORY_EXPORT void LibTokaMapFactoryLoader(libtokamap::FactoryEntryInterface& entry)
{
    std::lock_guard lock{g_load_mutex};
    if (g_impl_entry == nullptr) {
        if (!python_symbols_visible()) {
            load_python_library();
        }
        check_python_version();
        g_impl_entry = load_implementation();
    }
    g_impl_entry(entry);
}
