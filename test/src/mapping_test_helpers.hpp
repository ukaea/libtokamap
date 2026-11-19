#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"

namespace libtokamap::test
{

using MapEntries = std::unordered_map<std::string, std::unique_ptr<Mapping>>;

inline MapArguments make_map_arguments(MapEntries& entries)
{
    static nlohmann::json empty_global_data = nlohmann::json::object();

    // float, rank 1, no tracing/caching
    return MapArguments(entries, empty_global_data, DataType::Float, 1, false, false, nullptr);
}

} // namespace libtokamap::test
