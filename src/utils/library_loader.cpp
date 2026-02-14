#include "library_loader.hpp"

#ifdef _WIN32
#  include <Windows.h>
#  include <libloaderapi.h>
#else
#  include <dlfcn.h>
#endif
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "config.hpp"
#include "exceptions/exceptions.hpp"

namespace
{

using LibraryEntryFunction = void (*)(libtokamap::LibraryEntryInterface&);

constexpr const char* library_loader = "LibTokaMapLibraryLoader";
constexpr const char* factory_loader = "LibTokaMapFactoryLoader";

std::vector<libtokamap::LibraryFunction> load_library_functions(const std::filesystem::path& library_path)
{
#ifdef _WIN32
    HMODULE hModule = LoadLibraryA(library_path.string().c_str());
    if (hModule == nullptr) {
        throw libtokamap::TokaMapError("Failed to load library '" + library_path.string() + "'");
    }

    FARPROC function_pointer = GetProcAddress(hModule, library_loader);
#else
    void* handle = dlopen(library_path.c_str(), RTLD_LAZY);
    if (handle == nullptr) {
        throw libtokamap::TokaMapError("Failed to load library '" + library_path.string() + "'");
    }

    void* function_pointer = dlsym(handle, library_loader);
#endif
    if (function_pointer == nullptr) {
        throw libtokamap::TokaMapError("Failed to find entry function '" + std::string{ library_loader } + "'");
    }

    auto entry_function = reinterpret_cast<LibraryEntryFunction>(function_pointer);
    libtokamap::LibraryEntryInterface entry_interface;
    entry_function(entry_interface);

    return std::move(entry_interface.functions);
}

using FactoryEntryFunction = void (*)(libtokamap::FactoryEntryInterface&);

} // namespace

libtokamap::DataSourceFactory libtokamap::load_data_source_factory(const std::filesystem::path& library_path)
{
    void* handle = dlopen(library_path.c_str(), RTLD_LAZY);
    if (handle == nullptr) {
        throw libtokamap::TokaMapError("Failed to load library '" + library_path.string() + "'");
    }

    void* function_pointer = dlsym(handle, factory_loader);
    if (function_pointer == nullptr) {
        throw libtokamap::TokaMapError("Failed to find entry function '" + std::string{ factory_loader } + "'");
    }

    auto entry_function = reinterpret_cast<FactoryEntryFunction>(function_pointer);
    libtokamap::FactoryEntryInterface entry_interface;
    entry_function(entry_interface);

    return entry_interface.function;
}

std::vector<libtokamap::LibraryFunction>
libtokamap::load_custom_functions(const std::filesystem::path& custom_function_library)
{
    if (!std::filesystem::exists(custom_function_library)) {
        throw libtokamap::TokaMapError("Library path '" + custom_function_library.string() + "' does not exist");
    }
    if (!std::filesystem::is_regular_file(custom_function_library) || custom_function_library.extension() != LibrarySuffix) {
        throw libtokamap::TokaMapError("Invalid library path '" + custom_function_library.string() + "'");
    }
    return load_library_functions(custom_function_library);
}
