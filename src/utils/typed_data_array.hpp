#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <nlohmann/json.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "utils/compiler.hpp"

namespace libtokamap
{

enum class DataType : uint8_t {
    Unknown,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float,
    Double,
};

inline size_t data_type_size(DataType type)
{
    switch (type) {
        case DataType::Int8:
            return sizeof(int8_t);
        case DataType::Int16:
            return sizeof(int16_t);
        case DataType::Int32:
            return sizeof(int32_t);
        case DataType::Int64:
            return sizeof(int64_t);
        case DataType::UInt8:
            return sizeof(uint8_t);
        case DataType::UInt16:
            return sizeof(uint16_t);
        case DataType::UInt32:
            return sizeof(uint32_t);
        case DataType::UInt64:
            return sizeof(uint64_t);
        case DataType::Float:
            return sizeof(float);
        case DataType::Double:
            return sizeof(double);
        case DataType::Unknown:
            return 0;
    }
    LIBTOKAMAP_UNREACHABLE
}

template <typename T, int SIZE>
struct is_int : std::bool_constant<
    std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) * CHAR_BIT == SIZE
> {};

template <typename T, int SIZE>
struct is_uint : std::bool_constant<
    std::is_integral_v<T> && !std::is_signed_v<T> && sizeof(T) * CHAR_BIT == SIZE
> {};

template <typename T, int SIZE>
constexpr bool is_int_v = is_int<T, SIZE>::value;

template <typename T, int SIZE>
constexpr bool is_uint_v = is_uint<T, SIZE>::value;

template <typename T>
constexpr DataType data_type_of()
{
    if constexpr (is_int_v<T, 8>) { return DataType::Int8; }
    else if constexpr (is_int_v<T, 16>) { return DataType::Int16; }
    else if constexpr (is_int_v<T, 32>) { return DataType::Int32; }
    else if constexpr (is_int_v<T, 64>) { return DataType::Int64; }
    else if constexpr (is_uint_v<T, 8>) { return DataType::UInt8; }
    else if constexpr (is_uint_v<T, 16>) { return DataType::UInt16; }
    else if constexpr (is_uint_v<T, 32>) { return DataType::UInt32; }
    else if constexpr (is_uint_v<T, 64>) { return DataType::UInt64; }
    else if constexpr (std::is_same_v<T, float>) { return DataType::Float; }
    else if constexpr (std::is_same_v<T, double>) { return DataType::Double; }
    else { return DataType::Unknown; }
}

inline std::string data_type_name(DataType type)
{
    switch (type) {
        case DataType::Int8:    return "int8_t";
        case DataType::Int16:   return "int16_t";
        case DataType::Int32:   return "int32_t";
        case DataType::Int64:   return "int64_t";
        case DataType::UInt8:   return "uint8_t";
        case DataType::UInt16:  return "uint16_t";
        case DataType::UInt32:  return "uint32_t";
        case DataType::UInt64:  return "uint64_t";
        case DataType::Float:   return "float";
        case DataType::Double:  return "double";
        case DataType::Unknown: return "unknown";
    }
    LIBTOKAMAP_UNREACHABLE
}

class SubsetInfo
{
  public:
    SubsetInfo(std::optional<int64_t> start, std::optional<int64_t> stop, int64_t stride, size_t size)
        : m_stride{stride}, m_dim_size{size}
    {
        if (stride == 0) {
            throw libtokamap::ProcessingError{"stride of 0 is not allowed, apologies"};
        }

        // m_start
        if (!start.has_value()) {
            // If start omitted, need to default to values based on stride
            m_start = (stride > 0) ? 0 : m_dim_size - 1;
        } else if (start.value() < 0) {
            m_start = m_dim_size + start.value();
        } else {
            m_start = start.value();
        }

        // m_stop
        if (!stop.has_value()) {
            // If stop omitted, need to default to values based on stride
            if (stride > 0) {
                m_stop = m_dim_size;
            } else {
                // Dummy flag value to know when to go all the way to INCLUDE zeroth index
                m_stop = std::numeric_limits<uint64_t>::max();
            }
        } else if (stop.value() < 0) {
            m_stop = m_dim_size + stop.value();
        } else {
            m_stop = stop.value();
        }
    }

    [[nodiscard]] bool empty() const { return m_start == m_stop; }

    [[nodiscard]] uint64_t size() const {
        uint64_t size = 0;
        if (m_stride > 0) {
            if (m_start < m_stop) {
                size = (m_stop - m_start + m_stride - 1) / m_stride;
            }
        } else if (m_stride < 0) {
            if (m_stop == std::numeric_limits<uint64_t>::max()) {
                // As above
                size = (m_start + (-m_stride)) / (-m_stride);
            } else if (m_start > m_stop) {
                size = (m_start - m_stop - m_stride - 1) / (-m_stride);
            }
        }
        return size;
    }

    [[nodiscard]] bool validate() const
    {
        bool valid_stride = m_stride > 0
                            ? (m_start < m_dim_size && m_start <= m_stop && m_stop <= m_dim_size)
                            : (m_stop == std::numeric_limits<uint64_t>::max() || (m_stop <= m_start && m_start < m_dim_size));
        return valid_stride;
    }

    [[nodiscard]] uint64_t start() const { return m_start; }

    [[nodiscard]] uint64_t stop() const { return m_stop; }

    [[nodiscard]] int64_t stride() const { return m_stride; }

    [[nodiscard]] uint64_t dim_size() const { return m_dim_size; }

  private:
    uint64_t m_start;
    uint64_t m_stop;
    int64_t m_stride = 1;
    uint64_t m_dim_size;
};

std::vector<size_t> compute_offsets(const std::vector<size_t>& shape, const std::vector<SubsetInfo>& subsets);

class TypedDataArray
{
  public:
    TypedDataArray() : m_data_type{DataType::Unknown}, m_size{0}, m_owning{false} {}

    template <typename T>
    explicit TypedDataArray(const std::vector<T>& array, std::vector<size_t> shape = {})
        : m_data_type{data_type_of<T>()}, m_size{array.size()}, m_shape{std::move(shape)}, m_owning{true}
    {
        m_buffer = static_cast<char*>(malloc(m_size * sizeof(T)));
        std::memcpy(m_buffer, reinterpret_cast<const char*>(array.data()), m_size * sizeof(T));
        if (m_shape.empty()) {
            m_shape.push_back(m_size);
        }
    }

    template <typename T>
    explicit TypedDataArray(T* array, size_t size, std::vector<size_t> shape, bool owning = true)
        : m_data_type{data_type_of<T>()}, m_size{size}, m_shape{std::move(shape)}, m_owning{owning}
    {
        if (m_owning) {
            m_buffer = static_cast<char*>(malloc(m_size * sizeof(T)));
            std::memcpy(m_buffer, reinterpret_cast<const char*>(array), m_size * sizeof(T));
        } else {
            m_buffer = reinterpret_cast<char*>(array);
        }
    }

    template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    explicit TypedDataArray(const T value) : m_data_type{data_type_of<T>()}, m_size{1}, m_owning{true}
    {
        m_buffer = static_cast<char*>(malloc(sizeof(T)));
        std::memcpy(m_buffer, reinterpret_cast<const char*>(&value), sizeof(T));
    }

    explicit TypedDataArray(const std::string& value)
        : m_data_type{DataType::Int8}, m_size{value.size()}, m_shape{value.size()}, m_owning{true}
    {
        m_buffer = static_cast<char*>(malloc(m_size * sizeof(char)));
        std::memcpy(m_buffer, value.data(), m_size);
    }

    ~TypedDataArray()
    {
        if (m_owning) {
            free(m_buffer);
        }
    }

    [[nodiscard]] TypedDataArray clone() const
    {
        TypedDataArray clone;
        clone.m_data_type = m_data_type;
        clone.m_size = m_size;
        clone.m_shape = m_shape;
        clone.m_owning = true;
        size_t type_size = data_type_size(m_data_type);
        clone.m_buffer = static_cast<char*>(malloc(m_size * type_size));
        std::memcpy(clone.m_buffer, m_buffer, m_size * type_size);
        clone.m_trace = m_trace;
        return clone;
    }

    template <typename T> void apply(double scale_factor, double offset)
    {
        if (m_data_type != data_type_of<T>()) {
            throw libtokamap::DataTypeError{"invalid type given to apply"};
        }

        auto* data = reinterpret_cast<T*>(m_buffer);
        for (size_t idx = 0; idx < m_size; ++idx) {
            data[idx] = static_cast<T>((static_cast<double>(data[idx]) * scale_factor) + offset);
        }
    }

    template <typename T> void slice(const std::vector<SubsetInfo>& subsets)
    {
        if (m_data_type != data_type_of<T>()) {
            throw libtokamap::DataTypeError{"invalid type given to slice"};
        }
        if (subsets.size() != m_shape.size()) {
            throw libtokamap::ParameterError{"invalid number of subsets given"};
        }

        if (subsets.empty()) {
            return;
        }

        const size_t n_dims = m_shape.size();

        size_t new_size = 1;
        std::vector<size_t> new_shape;
        for (size_t dim = 0; dim < n_dims; ++dim) {
            auto len = subsets[dim].size();
            if (len > 1) {
                new_shape.push_back(len);
            }
            new_size *= len;
        }

        auto* array = reinterpret_cast<T*>(m_buffer);

        auto* new_buffer = static_cast<char*>(malloc(sizeof(T) * new_size));
        auto* new_array = reinterpret_cast<T*>(new_buffer);

        auto offsets = compute_offsets(m_shape, subsets);
        size_t idx = 0;
        for (const auto offset : offsets) {
            new_array[idx] = array[offset];
            ++idx;
        }

        if (m_owning) {
            free(m_buffer);
        }
        m_size = new_size;
        m_shape = new_shape;
        m_buffer = new_buffer;
        m_owning = true;
    }

    [[nodiscard]] bool empty() const { return m_size == 0; }

    [[nodiscard]] size_t size() const { return m_size; }

    [[nodiscard]] size_t rank() const { return m_shape.size(); }

    [[nodiscard]] DataType data_type() const { return m_data_type; }

    [[nodiscard]] const std::vector<size_t>& shape() const { return m_shape; }

    [[nodiscard]] char* buffer() const { return m_buffer; }

    [[nodiscard]] bool is_owning() const { return m_owning; }

    [[nodiscard]] char* release()
    {
        char* ptr = m_buffer;
        m_buffer = nullptr;
        m_owning = false;
        return ptr;
    }

    template <typename To, typename From> [[nodiscard]] TypedDataArray convert()
    {
        if (m_data_type != data_type_of<From>()) {
            throw libtokamap::DataTypeError{"invalid type given to convert"};
        }

        TypedDataArray new_array;

        new_array.m_data_type = data_type_of<To>();
        new_array.m_shape = m_shape;
        new_array.m_size = m_size;
        new_array.m_buffer = new char[m_size * sizeof(To)];
        new_array.m_owning = true;

        From* data = std::bit_cast<From*>(m_buffer);
        std::copy(data, data + m_size, std::bit_cast<To*>(new_array.m_buffer));

        return new_array;
    }

#if __cplusplus >= 202002L
    template <typename T> [[nodiscard]] std::span<T> span() const
    {
        if (m_data_type != data_type_of<T>()) {
            throw libtokamap::DataTypeError{"invalid type given to span"};
        }
        return std::span<T>{reinterpret_cast<T*>(m_buffer), m_size};
    }
#endif

    template <typename T> [[nodiscard]] const T* data() const
    {
        if (m_data_type != data_type_of<T>()) {
            throw libtokamap::DataTypeError{"invalid type given to data"};
        }
        return reinterpret_cast<T*>(m_buffer);
    }

    template <typename T> [[nodiscard]] std::vector<T> to_vector() const
    {
        if (m_data_type != data_type_of<T>()) {
            throw libtokamap::DataTypeError{"invalid type given to to_vector"};
        }
        const T* ptr = reinterpret_cast<T*>(m_buffer);
        return std::vector<T>{ptr, ptr + m_size};
    }

    [[nodiscard]] size_t element_size()
    {
        switch (m_data_type) {
            case DataType::Unknown:
                throw libtokamap::DataTypeError{"unknown data type"};
            case DataType::Int8:
                return sizeof(int8_t);
            case DataType::Int16:
                return sizeof(int16_t);
            case DataType::Int32:
                return sizeof(int32_t);
            case DataType::Int64:
                return sizeof(int64_t);
            case DataType::UInt8:
                return sizeof(uint8_t);
            case DataType::UInt16:
                return sizeof(uint16_t);
            case DataType::UInt32:
                return sizeof(uint32_t);
            case DataType::UInt64:
                return sizeof(uint64_t);
            case DataType::Float:
                return sizeof(float);
            case DataType::Double:
                return sizeof(double);
        }
    }

    constexpr static size_t default_max_elements = 10;
    constexpr static int default_precision = 3;

    [[nodiscard]] std::string to_string(size_t max_elements = default_max_elements,
                                        int precision = default_precision) const;

    // Moveable but not copyable
    TypedDataArray(const TypedDataArray&) = delete;
    TypedDataArray& operator=(const TypedDataArray&) = delete;

    TypedDataArray(TypedDataArray&& other) noexcept : TypedDataArray()
    {
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_data_type, other.m_data_type);
        std::swap(m_size, other.m_size);
        std::swap(m_shape, other.m_shape);
        std::swap(m_owning, other.m_owning);
        std::swap(m_trace, other.m_trace);
    };
    TypedDataArray& operator=(TypedDataArray&& other) noexcept
    {
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_data_type, other.m_data_type);
        std::swap(m_size, other.m_size);
        std::swap(m_shape, other.m_shape);
        std::swap(m_owning, other.m_owning);
        std::swap(m_trace, other.m_trace);
        return *this;
    };

    void set_trace(nlohmann::json trace) { m_trace = std::move(trace); }
    [[nodiscard]] const nlohmann::json& trace() const { return m_trace; }

  private:
    char* m_buffer = nullptr;
    DataType m_data_type;
    size_t m_size;
    std::vector<size_t> m_shape;
    bool m_owning;
    nlohmann::json m_trace;
};

} // namespace libtokamap
