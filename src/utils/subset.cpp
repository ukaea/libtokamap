#include "subset.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctre/ctre.hpp>
#include <optional>
#include <string>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "utils/typed_data_array.hpp"

namespace
{

int64_t to_int(std::optional<std::string> value, int64_t default_value)
{
    if (value && !value.value().empty()) {
        return std::stoi(value.value());
    }
    return default_value;
}

constexpr auto token_re = ctll::fixed_string{R"(\[([^\[\]]*)\])"};
constexpr auto index_re = ctll::fixed_string{R"((-?\d+))"};
constexpr auto slice_re = ctll::fixed_string{R"((-?\d*)(:(-?\d*)(:(-?\d*))?)?)"};

template <typename T> libtokamap::SubsetInfo parse_slice(const T& slice, size_t dimension)
{
    const auto& index_match = ctre::match<index_re>(slice);
    if (index_match) {
        int64_t index = to_int(index_match.template get<1>().to_optional_string(), 0);

        // AP: do we want to normalise here instead of in the actual subset class??
        auto normalised_index = (index < 0) ? dimension + index : index;
        auto subset = libtokamap::SubsetInfo{normalised_index, normalised_index + 1, 1, dimension};
        if (!subset.validate()) {
            throw libtokamap::ProcessingError{"invalid subset: " + slice.to_string()};
        }
        return subset;
    }
    const auto& slice_match = ctre::match<slice_re>(slice);
    if (slice_match) {
        int64_t stride = to_int(slice_match.template get<5>().to_optional_string(), 1);

        // m_start: if omitted, pass std::nullopt
        auto start_str = slice_match.template get<1>().to_optional_string();
        std::optional<int64_t> start;
        if (start_str && !start_str.value().empty()) {
            start = std::stoi(start_str.value());
        }

        // m_stop: if omitted, pass std::nullopt
        auto stop_str = slice_match.template get<3>().to_optional_string();
        std::optional<int64_t> stop;
        if (stop_str && !stop_str.value().empty()) {
            stop = std::stoi(stop_str.value());
        }

        auto subset = libtokamap::SubsetInfo{start, stop, stride, dimension};
        if (!subset.validate()) {
            throw libtokamap::ProcessingError{"invalid subset: " + slice.to_string()};
        }
        return subset;
    }
    throw libtokamap::ProcessingError{"invalid subset: " + slice.to_string()};
}

void apply_subset(libtokamap::TypedDataArray& input, const std::optional<std::string>& slice)
{
    if (!slice) {
        return;
    }

    std::vector<libtokamap::SubsetInfo> subset_info = libtokamap::parse_slices(slice.value(), input.shape());
    using libtokamap::DataType;
    switch (input.data_type()) {
        case DataType::Int8:
            input.slice<int8_t>(subset_info);
            break;
        case DataType::Int16:
            input.slice<int16_t>(subset_info);
            break;
        case DataType::Int32:
            input.slice<int32_t>(subset_info);
            break;
        case DataType::Int64:
            input.slice<int64_t>(subset_info);
            break;
        case DataType::UInt8:
            input.slice<uint8_t>(subset_info);
            break;
        case DataType::UInt16:
            input.slice<uint16_t>(subset_info);
            break;
        case DataType::UInt32:
            input.slice<uint32_t>(subset_info);
            break;
        case DataType::UInt64:
            input.slice<uint64_t>(subset_info);
            break;
        case DataType::Float:
            input.slice<float>(subset_info);
            break;
        case DataType::Double:
            input.slice<double>(subset_info);
            break;
        default:
            throw libtokamap::DataTypeError{"unsupported data type"};
    }
}

void apply_scale_offset(libtokamap::TypedDataArray& input, std::optional<float> scale_factor,
                        std::optional<float> offset)
{
    if (input.empty() || (!scale_factor && !offset)) {
        return;
    }
    using libtokamap::DataType;
    switch (input.data_type()) {
        case DataType::Int8:
            input.apply<int8_t>(scale_factor.value_or(1.0), offset.value_or(0.0));
            break;
        case DataType::Int16:
            input.apply<int16_t>(scale_factor.value_or(1.0), offset.value_or(0.0));
            break;
        case DataType::Int32:
            input.apply<int32_t>(scale_factor.value_or(1.0), offset.value_or(0.0));
            break;
        case DataType::Int64:
            input.apply<int64_t>(scale_factor.value_or(1.0), offset.value_or(0.0));
            break;
        case DataType::UInt8:
            input.apply<uint8_t>(scale_factor.value_or(1.0), offset.value_or(0.0));
            break;
        case DataType::UInt16:
            input.apply<uint16_t>(scale_factor.value_or(1.0), offset.value_or(0.0));
            break;
        case DataType::UInt32:
            input.apply<uint32_t>(scale_factor.value_or(1.0), offset.value_or(0.0));
            break;
        case DataType::UInt64:
            input.apply<uint64_t>(scale_factor.value_or(1.0), offset.value_or(0.0));
            break;
        case DataType::Float:
            input.apply<float>(scale_factor.value_or(1.0), offset.value_or(0.0));
            break;
        case DataType::Double:
            input.apply<double>(scale_factor.value_or(1.0), offset.value_or(0.0));
            break;
        default:
            throw libtokamap::DataTypeError{"unsupported data type"};
    }
}

} // namespace

std::vector<libtokamap::SubsetInfo> libtokamap::parse_slices(const std::string& slice, const std::vector<size_t>& shape)
{
    // Special case for scalar data with [0] slice - should return empty subsets (no slicing needed)
    if (shape.empty() && slice == "[0]") {
        return {};
    }

    size_t dim_idx = 0;
    std::vector<libtokamap::SubsetInfo> subsets;
    for (const auto& token : ctre::search_all<token_re>(slice)) {
        if (dim_idx == shape.size()) {
            throw libtokamap::ParameterError{"too many slices provided"};
        }
        subsets.push_back(parse_slice(token.get<1>(), shape[dim_idx]));
        ++dim_idx;
    }
    return subsets;
}

void libtokamap::update_array(libtokamap::TypedDataArray& input, const std::optional<std::string>& slice,
                              std::optional<float> scale_factor, std::optional<float> offset)
{
    apply_subset(input, slice);
    apply_scale_offset(input, scale_factor, offset);
}
