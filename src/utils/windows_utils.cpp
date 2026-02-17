#include "os_utils.hpp"

#include <Windows.h>
#include <DbgHelp.h>
#include <LibLoaderAPI.h>
#include <string>
#include <filesystem>

#include "exceptions/exceptions.hpp"

std::string libtokamap::demangle(const char* name)
{
    char buffer[1024];
    UnDecorateSymbolName(name, buffer, sizeof(buffer), UNDNAME_COMPLETE);
    return buffer;
}

void* libtokamap::load_library_object(const std::filesystem::path& library_path, const std::string& symbol_name)
{
    HMODULE hModule = LoadLibraryA(library_path.string().c_str());
    if (hModule == nullptr) {
        throw libtokamap::TokaMapError("Failed to load library '" + library_path.string() + "'");
    }

    FARPROC function_pointer = GetProcAddress(hModule, symbol_name.c_str());
    return function_pointer;
}
