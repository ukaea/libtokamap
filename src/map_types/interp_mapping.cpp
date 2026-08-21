#include "map_types/interp_mapping.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/typed_data_array.hpp"

namespace
{

using libtokamap::DataType;
using libtokamap::InterpType;
using libtokamap::TypedDataArray;

std::vector<double> to_doubles(const TypedDataArray& array, const std::string& name)
{
    if (array.data_type() == DataType::Float) {
        const auto* data = array.data<float>();
        return std::vector<double>{data, data + array.size()};
    }
    if (array.data_type() == DataType::Double) {
        return array.to_vector<double>();
    }
    throw libtokamap::DataTypeError{"INTERP mapping '" + name + "' must be floating point, got " +
                                    libtokamap::data_type_name(array.data_type())};
}

void check_increasing(const std::vector<double>& base, const std::string& name)
{
    for (size_t idx = 1; idx < base.size(); ++idx) {
        if (base[idx] <= base[idx - 1]) {
            throw libtokamap::ProcessingError{"INTERP base '" + name +
                                              "' must be strictly increasing, but is not at index " +
                                              std::to_string(idx)};
        }
    }
}

/**
 * @brief Indices of the two base points either side of the given point
 *
 * Points beyond either end of the base are clamped rather than extrapolated,
 * and come back as a pair of equal indices for the caller to take as-is
 */
std::pair<size_t, size_t> bracket(double point, const std::vector<double>& base)
{
    // clamp, no extrapolate
    if (point <= base.front()) {
        return {0, 0};
    }
    if (point >= base.back()) {
        const size_t last = base.size() - 1;
        return {last, last};
    }

    const auto upper = std::ranges::upper_bound(base, point);
    const size_t high = static_cast<size_t>(upper - base.begin());
    return {high - 1, high};
}

/**
 * @brief Straight line between each pair of bracketing base points
 *
 * Every target point is an independent lookup into the base, so the targets
 * need not be ordered
 */
std::vector<double> interpolate_linear(const std::vector<double>& target, const std::vector<double>& base,
                                       const std::vector<double>& input)
{
    std::vector<double> interpolated(target.size());
    std::ranges::transform(target, interpolated.begin(), [&](double point) {
        const auto [low, high] = bracket(point, base);
        if (low == high) {
            return input[low];
        }
        // fraction of the way between the bracketing base points
        const double fraction = (point - base[low]) / (base[high] - base[low]);
        return std::lerp(input[low], input[high], fraction);
    });
    return interpolated;
}

/**
 * @brief Interpolate the input onto the target points, the base being the
 * reference the input is defined against
 *
 * Each interpolation type owns its own loop, so that types needing a setup pass
 * over the whole base (splines, for instance) have somewhere to do it once
 *
 * @param interp_type which interpolation to run, eg. linear
 * @param target points to evaluate at, in any order
 * @param base axis the input is sampled on, strictly increasing and the same
 *             length as the input
 * @param input data values being interpolated, one per base point
 * @return one interpolated value per target point, in target order
 */
std::vector<double> interpolate(InterpType interp_type, const std::vector<double>& target,
                                const std::vector<double>& base, const std::vector<double>& input)
{
    switch (interp_type) {
        case InterpType::LINEAR:
            return interpolate_linear(target, base, input);
        case InterpType::UNKNOWN:
            throw libtokamap::ProcessingError{"Unknown interpolation type"};
        // AJP: add more later
    }
    LIBTOKAMAP_UNREACHABLE
}

} // namespace

libtokamap::TypedDataArray libtokamap::InterpMapping::map_interp_args(const MapArguments& arguments,
                                                                      const std::string& name) const
{
    if (!arguments.entries.contains(name)) {
        throw libtokamap::MappingError{"Mapping '" + name + "' referenced by INTERP mapping not found"};
    }
    return arguments.entries.at(name)->map(arguments);
}

libtokamap::TypedDataArray libtokamap::InterpMapping::map(const MapArguments& arguments) const
{

    const auto input_array = map_interp_args(arguments, m_input);
    const auto base_array = map_interp_args(arguments, m_base);
    const auto target_array = map_interp_args(arguments, m_target);

    // empty check
    if (input_array.empty() || base_array.empty() || target_array.empty()) {
        throw libtokamap::ProcessingError{"One (or more) of the INTERP parameters are empty"};
    }

    // rank 1D check
    if (input_array.rank() != 1 || base_array.rank() != 1 || target_array.rank() != 1) {
        throw libtokamap::ProcessingError{"Only 1D interpolation is supported"
					  " - please ensure all INTERP parameters are rank 1"};
    }

    // input size match == base size check
    if (input_array.size() != base_array.size()) {
        throw libtokamap::ProcessingError{"INTERP data '" + m_input + "' has " + std::to_string(input_array.size()) +
                                          " elements but base '" + m_base + "' has " +
                                          std::to_string(base_array.size())};
    }

    // Error for single point array check
    if (base_array.size() < 2) {
        throw libtokamap::ProcessingError{"INTERP base '" + m_base +
                                          "' must have at least 2 points, got " +
                                          std::to_string(base_array.size())};
    }

    const auto input = to_doubles(input_array, m_input);
    const auto base = to_doubles(base_array, m_base);
    const auto target = to_doubles(target_array, m_target);

    // ascending check
    check_increasing(base, m_base);

    const auto interpolated = interpolate(m_interp_type, target, base, input);

    // use float if originally float
    if (input_array.data_type() == DataType::Float) {
        std::vector<float> result;
        result.reserve(interpolated.size());

        std::transform(
            interpolated.begin(),
            interpolated.end(),
            std::back_inserter(result),
            [](double value) { return static_cast<float>(value); });

        return TypedDataArray{std::move(result)};
    }
    return TypedDataArray{std::move(interpolated)};
}
