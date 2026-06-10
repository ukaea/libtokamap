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

libtokamap::TypedDataArray libtokamap::DataSourceMapping::map(const MapArguments& arguments) const
{
    LIBTOKAMAP_PROFILER(profiler);

    TypedDataArray array;

    DataSourceArgs args = m_data_source_args;
    for (auto& [key, value] : args) {
        if (value.is_string()) {
            value = libtokamap::render(value.get<std::string>(), arguments.global_data);
        }
    }
    LIBTOKAMAP_PROFILER_ATTR(profiler, "args", args);

    const DataSourceCacheKey cache_key = {m_data_source, m_name, args};
    if (arguments.data_source_cache_enabled && arguments.data_source_cache != nullptr) {
        auto cached = arguments.data_source_cache->get(cache_key);
        if (cached.has_value()) {
            array = std::move(cached.value());
            LIBTOKAMAP_PROFILER_ATTR(profiler, "cache_hit", true);
        } else {
            array = m_data_source->get(args, arguments, arguments.ram_cache);
            arguments.data_source_cache->put(cache_key, array);
            LIBTOKAMAP_PROFILER_ATTR(profiler, "cache_hit", false);
        }
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
}
