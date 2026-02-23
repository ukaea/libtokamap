#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <libtokamap.hpp>
#include <nlohmann/json.hpp>
#include <string>

#include <vector>

#include "config_path.hpp"

namespace
{

uint64_t map(libtokamap::MappingHandler& mapping_handler, nlohmann::json& trace, const std::string& mapping,
             const std::string& path)
{
    libtokamap::DataType data_type = libtokamap::DataType::Int8;
    int rank = 1;
    nlohmann::json extra_attributes = {{"shot", 42}};

    auto result = mapping_handler.map(mapping, path, data_type, rank, extra_attributes);

    constexpr int64_t key_length = 35;
    std::string padding;
    padding.resize(std::max(key_length - static_cast<int64_t>(path.size()), int64_t{0}), ' ');
    std::cout << path << padding << " -> " << result.to_string() << "\n";

    if (!result.trace().empty()) {
        trace[path] = result.trace();
    }

    if (result.data_type() == libtokamap::DataType::UInt64) {
        return *std::bit_cast<uint64_t*>(result.buffer());
    }
    return 0;
}

nlohmann::json map_all(libtokamap::MappingHandler& mapping_handler, const std::string& mapping)
{
    nlohmann::json trace;

    map(mapping_handler, trace, mapping, "magnetics/version");
    auto num_coils = map(mapping_handler, trace, mapping, "magnetics/coil");
    for (auto coil = 0UL; coil < num_coils; ++coil) {
        std::string path = "magnetics/coil[" + std::to_string(coil) + "]";
        map(mapping_handler, trace, mapping, path + "/name");
        map(mapping_handler, trace, mapping, path + "/area");
        auto num_positions = map(mapping_handler, trace, mapping, path + "/position");
        for (auto position = 0UL; position < num_positions; ++position) {
            std::string pos_path = path + ("/position[" + std::to_string(position) + "]");
            map(mapping_handler, trace, mapping, pos_path + "/r");
            map(mapping_handler, trace, mapping, pos_path + "/z");
        }
        map(mapping_handler, trace, mapping, path + "/flux/time");
        map(mapping_handler, trace, mapping, path + "/flux/data");
        map(mapping_handler, trace, mapping, path + "/flux/data_scaled");
        map(mapping_handler, trace, mapping, path + "/flux/dot_product");
    }

    return trace;
}

} // namespace

int main()
{
    libtokamap::Profiler::init();

    try {
        libtokamap::MappingHandler mapping_handler;

        std::filesystem::path config_dir = ConfigDirectory;
        std::filesystem::path config_path = config_dir / "config.toml";
        mapping_handler.init(config_path);

        const char* experiment = "EXAMPLE";
        auto trace = map_all(mapping_handler, experiment);

        std::ofstream("trace.json") << trace.dump(4);
    } catch (std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }

    libtokamap::Profiler::write("profile.json");

    return 0;
}
