#pragma once

#include <filesystem>
#include <string>

namespace libtokamap {
    std::string demangle(const char* name);
    void* load_library_object(const std::filesystem::path& library_path, const std::string& symbol_name);
}
