#include "map_types/dim_mapping.hpp"

#include <cstdint>
#include <cstdlib>

#include "exceptions/exceptions.hpp"
#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/profiler.hpp"
#include "utils/typed_data_array.hpp"

libtokamap::TypedDataArray libtokamap::DimMapping::map(const MapArguments& arguments) const
{
    LIBTOKAMAP_PROFILER(profiler);

    if (!arguments.entries.contains(m_dim_probe)) {
        throw libtokamap::MappingError{"invalid DIM_PROBE '" + m_dim_probe + "'"};
    }
    LIBTOKAMAP_PROFILER_ATTR(profiler, "dim_probe", m_dim_probe);

    // TODO: Add DIM_INDEX for selecting which dimension to return size of
    constexpr int dim_index = 0;

    auto array = arguments.entries.at(m_dim_probe)->map(arguments);
    if (array.rank() == 0) {
        // Special case for scalar arrays
        return TypedDataArray{static_cast<uint64_t>(1)};

        // ** commented out for now to avoid warnings while dim_index is const **
        //if (dim_index == 0) {
        //    // Special case for scalar arrays
        //    return TypedDataArray{static_cast<uint64_t>(1)};
        //}
        //throw libtokamap::MappingError{"cannot use DIM_PROBE on rank 0 mapping '" + m_dim_probe + "'"};
    }

    auto result = TypedDataArray{static_cast<uint64_t>(array.shape()[dim_index])};
    if (arguments.trace_enabled) {
        result.set_trace({{"map_type", "dimension"}, {"dim_of", array.trace()}});
    }
    return result;
}
