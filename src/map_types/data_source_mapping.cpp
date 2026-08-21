#include "data_source_mapping.hpp"

#include <cstddef>
#include <functional>
#include <inja/inja.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "exceptions/exceptions.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/render.hpp"
#include "utils/subset.hpp"
#include "utils/typed_data_array.hpp"
#include "utils/profiler.hpp"

std::unordered_map<libtokamap::DataSourceCacheKey, int> libtokamap::DataSourceMapping::m_data_source_count = {};
std::unordered_map<libtokamap::DataSourceCacheKey, libtokamap::TypedDataArray>
    libtokamap::DataSourceMapping::m_data_source_cache = {};

libtokamap::TypedDataArray libtokamap::DataSourceMapping::map(const MapArguments& arguments) const
{
    LIBTOKAMAP_PROFILER(profiler);

    TypedDataArray array;

    DataSourceArgs args = m_data_source_args;
    for (const auto& [key, value] : arguments.runtime_attributes.items()) {
        if (!args.contains(key) && value.is_primitive() && !value.is_null()) {
            args[key] = value;
        }
    }
    for (auto& [key, value] : args) {
        if (value.is_string()) {
            value = libtokamap::render(value.get<std::string>(), arguments.global_data);
        }
    }
    LIBTOKAMAP_PROFILER_ATTR(profiler, "args", args);

    DataSourceCacheKey cache_key = {m_name, args};
    bool cache_hit = false;

    if (arguments.cache_enabled && m_data_source_cache.contains(cache_key)) {
        array = m_data_source_cache.at(cache_key).clone();
        cache_hit = true;
        LIBTOKAMAP_PROFILER_ATTR(profiler, "cache_hit", true);
    } else {
        array = m_data_source->get(args, arguments, arguments.ram_cache);
        LIBTOKAMAP_PROFILER_ATTR(profiler, "cache_hit", false);
    }

    // Render the slice string at runtime if it exists
    std::optional<std::string> rendered_slice;
    if (m_slice.has_value()) {
        rendered_slice = libtokamap::render(m_slice.value(), arguments.global_data);
    }

    update_array(array, rendered_slice, m_scale, m_offset);
    if (arguments.trace_enabled) {
        nlohmann::json trace;
        trace["map_type"] = "data_source";
        trace["data_source"] = {{m_name, array.trace()}};
        trace["arguments"] = args;
        if (m_scale) {
            trace["scale"] = m_scale.value();
        }
        if (m_offset) {
            trace["offset"] = m_offset.value();
        }
        if (m_slice) {
            trace["slice"] = m_slice.value();
        }
        array.set_trace(trace);
    }

    DataSourceCacheKey unrendered_cache_key = {m_name, m_data_source_args};
    if (!cache_hit && m_data_source_count.at(unrendered_cache_key) >= CacheThreshold) {
        m_data_source_cache[cache_key] = array.clone();
    }

    return array;
}

libtokamap::DataSourceMapping::DataSourceMapping(DataSourceName name, DataSource* data_source,
                                                 DataSourceArgs data_source_args, std::optional<float> offset,
                                                 std::optional<float> scale, std::optional<std::string> slice)
    : m_name{std::move(name)}, m_data_source{data_source}, m_data_source_args{std::move(data_source_args)},
      m_offset{offset}, m_scale{scale}, m_slice{std::move(slice)}
{
    if (data_source == nullptr) {
        throw TokaMapError{"data_source is nullptr"};
    }
    DataSourceCacheKey key{m_name, m_data_source_args};
    m_data_source_count[key]++;
}

namespace
{

// taken from boost hash_combine
template <class T> inline void hash_combine(std::size_t& seed, const T& value)
{
    constexpr std::size_t offset = 0x9e3779b9;
    constexpr std::size_t left_shift = 6;
    constexpr std::size_t right_shift = 2;

    std::hash<T> hasher;
    seed ^= hasher(value) + offset + (seed << left_shift) + (seed >> right_shift);
}

} // namespace

size_t std::hash<libtokamap::DataSourceCacheKey>::operator()(const libtokamap::DataSourceCacheKey& key) const
{
    size_t hash = 0;
    hash_combine(hash, key.first);
    hash_combine(hash, key.second);
    return hash;
}
