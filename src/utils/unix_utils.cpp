#include "os_utils.hpp"

#include <cxxabi.h>
#include <dlfcn.h>
#include <filesystem>
#include <string>

#include "exceptions/exceptions.hpp"

std::string libtokamap::demangle(const char* name)
{
    int status = 0;
    return abi::__cxa_demangle(name, nullptr, nullptr, &status);
}

void* libtokamap::load_library_object(const std::filesystem::path& library_path, const std::string& symbol_name)
{
    void* handle = dlopen(library_path.c_str(), RTLD_LAZY);
    if (handle == nullptr) {
        throw libtokamap::TokaMapError("Failed to load library '" + library_path.string() + "'");
    }

    void* function_pointer = dlsym(handle, symbol_name.c_str());
    return function_pointer;
}
