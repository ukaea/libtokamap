#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

#include "utils/typed_data_array.hpp"

namespace libtokamap
{

class RamCache;
class Mapping;

struct MapArguments {
    const std::unordered_map<std::string, std::unique_ptr<Mapping>>& entries;
    const nlohmann::json& global_data;
    DataType data_type;
    int rank;
    bool trace_enabled;
    bool cache_enabled;
    RamCache* ram_cache;
    const nlohmann::json& runtime_attributes;

    explicit MapArguments(const std::unordered_map<std::string, std::unique_ptr<Mapping>>& entries,
                          const nlohmann::json& global_data, const DataType data_type, const int rank,
                          const bool trace_enabled, const bool cache_enabled, RamCache* ram_cache)
        : MapArguments(entries, global_data, data_type, rank, trace_enabled, cache_enabled, ram_cache,
                       empty_runtime_attributes())
    {
    }

    explicit MapArguments(const std::unordered_map<std::string, std::unique_ptr<Mapping>>& entries,
                          const nlohmann::json& global_data, const DataType data_type, const int rank,
                          const bool trace_enabled, const bool cache_enabled, RamCache* ram_cache,
                          const nlohmann::json& runtime_attributes)
        : entries{entries}, global_data{global_data}, data_type{data_type}, rank{rank}, trace_enabled{trace_enabled},
          cache_enabled{cache_enabled}, ram_cache{ram_cache}, runtime_attributes{runtime_attributes}
    {
    }

  private:
    static const nlohmann::json& empty_runtime_attributes()
    {
        static const nlohmann::json empty = nlohmann::json::object();
        return empty;
    }
};

} // namespace libtokamap
