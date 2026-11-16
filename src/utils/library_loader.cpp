#include "library_loader.hpp"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "config.hpp"
#include "exceptions/exceptions.hpp"
#include "utils/os_utils.hpp"

namespace
{

using LibraryEntryFunction = void (*)(libtokamap::LibraryEntryInterface&);

constexpr const char* library_loader = "LibTokaMapLibraryLoader";
constexpr const char* factory_loader = "LibTokaMapFactoryLoader";
constexpr const char* factory_abi_version = "LibTokaMapFactoryABIVersion";

std::vector<libtokamap::LibraryFunction> load_library_functions(const std::filesystem::path& library_path)
{
    void* function_pointer = libtokamap::load_library_object(library_path, library_loader);

    if (function_pointer == nullptr) {
        throw libtokamap::TokaMapError("Failed to find entry function '" + std::string{ library_loader } + "'");
    }

    auto entry_function = reinterpret_cast<LibraryEntryFunction>(function_pointer);
    libtokamap::LibraryEntryInterface entry_interface;
    entry_function(entry_interface);

    return std::move(entry_interface.functions);
}

using FactoryEntryFunction = void (*)(libtokamap::FactoryEntryInterface&);
using FactoryABIVersionFunction = int (*)();

void check_factory_abi_version(const std::filesystem::path& library_path)
{
    void* function_pointer = libtokamap::load_library_object(library_path, factory_abi_version);
    if (function_pointer == nullptr) {
        return;
    }
    auto version_function = reinterpret_cast<FactoryABIVersionFunction>(function_pointer);
    int version = version_function();
    if (version != libtokamap::FactoryABIVersion) {
        throw libtokamap::ConfigurationError("Data source factory '" + library_path.string() +
                                             "' was built for factory ABI version " + std::to_string(version) +
                                             " but this LibTokaMap uses version " +
                                             std::to_string(libtokamap::FactoryABIVersion));
    }
}

} // namespace

libtokamap::DataSourceFactory libtokamap::load_data_source_factory(const std::filesystem::path& library_path)
{
    check_factory_abi_version(library_path);

    void* function_pointer = load_library_object(library_path, factory_loader);
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
