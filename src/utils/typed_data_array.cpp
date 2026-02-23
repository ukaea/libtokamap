#include "typed_data_array.hpp"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

using Indices = std::vector<size_t>;
using IndicesList = std::vector<Indices>;

// Generate all index tuples for given start, stop, stride arrays
IndicesList generate_indices(const std::vector<libtokamap::SubsetInfo>& subsets)
{
    size_t n_dims = subsets.size();
    IndicesList result;
    Indices current(n_dims);

    // Initialize current index to start
    for (size_t i = 0; i < n_dims; ++i) {
        current[i] = subsets[i].start();
    }

    bool done = false;
    while (!done) {
        result.push_back(current);

        // Increment the last dimension and carry over if needed
        for (int64_t i = static_cast<int64_t>(n_dims) - 1; i >= 0; --i) {
            current[i] += subsets[i].stride();

            bool within_bounds = false;
            if (subsets[i].stride() > 0) {
                within_bounds = (current[i] < subsets[i].stop());
            } else {
                // For negative stride, stop can be UINT64_MAX - handle separately!
                if (subsets[i].stop() == std::numeric_limits<uint64_t>::max()) {
                    // Go down to 0 inclusive
                    within_bounds = (current[i] < subsets[i].dim_size());
                } else {
                    within_bounds = (current[i] <= subsets[i].dim_size() && current[i] > subsets[i].stop());
                }
            }

            if (within_bounds) {
                break;
            }
            if (i == 0) {
                done = true;
            } else {
                current[i] = subsets[i].start();
            }
        }
    }

    return result;
}

size_t compute_offset(const Indices& indices, const std::vector<size_t>& index_factors)
{
    size_t offset = 0;
    for (size_t i = 0; i < indices.size(); ++i) {
        offset += indices[i] * index_factors[i];
    }
    return offset;
}

std::vector<size_t> compute_index_factors(const std::vector<size_t>& shape)
{
    size_t n_dims = shape.size();
    std::vector<size_t> factors(n_dims);
    factors[n_dims - 1] = 1;
    for (int64_t i = static_cast<int64_t>(n_dims) - 2; i >= 0; --i) {
        factors[i] = factors[i + 1] * shape[i + 1];
    }
    return factors;
}

std::ostream& operator<<(std::ostream& out, const std::vector<size_t>& shape)
{
    out << "[";
    const char* delim = "";
    for (const auto element : shape) {
        out << delim << element;
        delim = ",";
    }
    out << "]";
    return out;
}

template <typename T> struct SpanStreamAdaptor {
    std::span<const T> span;
    size_t max_elements;
    int precision;
};

template <typename T>
    requires(std::is_arithmetic_v<T>)
std::ostream& operator<<(std::ostream& out, const SpanStreamAdaptor<T>& data)
{
    out << "[";
    out << std::setprecision(data.precision);
    const char* delim = "";
    const size_t max = data.max_elements;
    size_t count = 0;
    for (const auto element : data.span) {
        if (count == max) {
            out << ",...";
            break;
        }
        if constexpr (std::is_same_v<T, char>) {
            if (isprint(element)) {
                out << delim << element;
            } else {
                out << delim << "\\x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(element);
            }
        } else {
            out << delim << element;
        }
        delim = ",";
        ++count;
    }
    out << "]";
    return out;
}



template <typename T> void print(std::ostream& out, const char* buffer, size_t size, size_t max_elements, int precision)
{
    std::span<const T> data{std::bit_cast<const T*>(buffer), size};
    SpanStreamAdaptor<T> adaptor{data, max_elements, precision};
    out << ", data=" << adaptor;
}

} // namespace

std::vector<size_t> libtokamap::compute_offsets(const std::vector<size_t>& shape,
                                                const std::vector<SubsetInfo>& subsets)
{
    auto indices_list = generate_indices(subsets);
    auto index_factors = compute_index_factors(shape);

    std::vector<size_t> offsets;
    for (const auto& indices : indices_list) {
        offsets.push_back(compute_offset(indices, index_factors));
    }
    return offsets;
}

std::string libtokamap::TypedDataArray::to_string(size_t max_elements, int precision) const
{
    std::stringstream out;
    out << "{ type=" << data_type_name(m_data_type) << ", size=" << m_size << ", shape=" << m_shape;
    switch (m_data_type) {
        case DataType::Int8:
            print<int8_t>(out, m_buffer, m_size, max_elements, precision);
            break;
        case DataType::Int16:
            print<int16_t>(out, m_buffer, m_size, max_elements, precision);
            break;
        case DataType::Int32:
            print<int32_t>(out, m_buffer, m_size, max_elements, precision);
            break;
        case DataType::Int64:
            print<int64_t>(out, m_buffer, m_size, max_elements, precision);
            break;
        case DataType::UInt8:
            print<uint8_t>(out, m_buffer, m_size, max_elements, precision);
            break;
        case DataType::UInt16:
            print<uint16_t>(out, m_buffer, m_size, max_elements, precision);
            break;
        case DataType::UInt32:
            print<uint32_t>(out, m_buffer, m_size, max_elements, precision);
            break;
        case DataType::UInt64:
            print<uint64_t>(out, m_buffer, m_size, max_elements, precision);
            break;
        case DataType::Float:
            print<float>(out, m_buffer, m_size, max_elements, precision);
            break;
        case DataType::Double:
            print<double>(out, m_buffer, m_size, max_elements, precision);
            break;
        default:
            throw libtokamap::DataTypeError{"unhandled data type: '" + data_type_name(m_data_type) + "'"};
    }
    out << " }";
    return out.str();
}
