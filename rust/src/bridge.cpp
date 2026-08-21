#include "bridge.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iterator>
#include <libtokamap.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <rust/cxx.h>
#include <stdexcept>
#include <string>

#include "libtokamap-rust/src/lib.rs.h"

void MappingHandlerWrapper::reset()
{
    try {
        handler.reset();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Reset failed: ") + e.what());
    }
}

void MappingHandlerWrapper::init_with_path(rust::Str config_path)
{
    try {
        std::filesystem::path path(static_cast<std::string>(config_path));
        handler.init(path);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Initialization with path failed: ") + e.what());
    }
}

void MappingHandlerWrapper::init_with_json(rust::Str config_json)
{
    try {
        nlohmann::json config = nlohmann::json::parse(static_cast<std::string>(config_json));
        handler.init(config);
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Initialization with JSON failed: ") + e.what());
    }
}

RustTypedDataArray MappingHandlerWrapper::map_data(rust::Str experiment, rust::Str path, uint32_t data_type_index,
    int32_t rank,
    rust::Str extra_attributes)
{
    try {
        // Parse extra attributes JSON
        nlohmann::json attrs;
        if (!extra_attributes.empty() && extra_attributes != "{}") {
            attrs = nlohmann::json::parse(extra_attributes);
        }

        // Convert data type index to DataType
        libtokamap::DataType data_type = data_type_index_from_u32(data_type_index);

        // Call the C++ map function
        auto cpp_array =
            handler.map(static_cast<std::string>(experiment), static_cast<std::string>(path), data_type, rank, attrs);

        // Convert to Rust-compatible format
        return convert_typed_data_array(cpp_array);

    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error(std::string("JSON parse error in extra_attributes: ") + e.what());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Mapping failed: ") + e.what());
    }
}

void MappingHandlerWrapper::register_data_source_factory_with_lib(rust::Str factory_name, rust::Str library_path)
{
    try {
        std::filesystem::path lib_path(static_cast<std::string>(library_path));
        handler.register_data_source_factory(static_cast<std::string>(factory_name),
                                             static_cast<std::string>(lib_path));
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Data source factory registration failed: ") + e.what());
    }
}

void MappingHandlerWrapper::register_data_source_with_factory(rust::Str name, rust::Str factory_name,
                                                              const RustDataSourceFactoryArgs& args)
{
    try {
        auto cpp_args = convert_factory_args(args);
        handler.register_data_source(static_cast<std::string>(name), static_cast<std::string>(factory_name), cpp_args);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Data source registration failed: ") + e.what());
    }
}

void MappingHandlerWrapper::unregister_data_source(rust::Str name)
{
    try {
        handler.unregister_data_source(static_cast<std::string>(name));
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Data source unregistration failed: ") + e.what());
    }
}

void MappingHandlerWrapper::load_custom_function_library(rust::Str library_path)
{
    try {
        std::filesystem::path lib_path(static_cast<std::string>(library_path));
        handler.load_custom_function_library(lib_path);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Custom function library loading failed: ") + e.what());
    }
}

void MappingHandlerWrapper::unregister_custom_function(rust::Str library_name, rust::Str function_name)
{
    try {
        handler.unregister_custom_function(static_cast<std::string>(library_name),
                                           static_cast<std::string>(function_name));
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Custom function unregistration failed: ") + e.what());
    }
}

// Factory function
std::unique_ptr<MappingHandlerWrapper> new_mapping_handler() { return std::make_unique<MappingHandlerWrapper>(); }

// Helper function to convert data type index to DataType
libtokamap::DataType data_type_index_from_u32(uint32_t data_type)
{
    constexpr uint32_t INT8_TYPE = 1;
    constexpr uint32_t INT16_TYPE = 2;
    constexpr uint32_t INT32_TYPE = 3;
    constexpr uint32_t INT64_TYPE = 4;
    constexpr uint32_t UINT8_TYPE = 5;
    constexpr uint32_t UINT16_TYPE = 6;
    constexpr uint32_t UINT32_TYPE = 7;
    constexpr uint32_t UINT64_TYPE = 8;
    constexpr uint32_t FLOAT_TYPE = 9;
    constexpr uint32_t DOUBLE_TYPE = 10;

    switch (data_type) {
        case INT8_TYPE:   return libtokamap::DataType::Int8;
        case INT16_TYPE:  return libtokamap::DataType::Int16;
        case INT32_TYPE:  return libtokamap::DataType::Int32;
        case INT64_TYPE:  return libtokamap::DataType::Int64;
        case UINT8_TYPE:  return libtokamap::DataType::UInt8;
        case UINT16_TYPE: return libtokamap::DataType::UInt16;
        case UINT32_TYPE: return libtokamap::DataType::UInt32;
        case UINT64_TYPE: return libtokamap::DataType::UInt64;
        case FLOAT_TYPE:  return libtokamap::DataType::Float;
        case DOUBLE_TYPE: return libtokamap::DataType::Double;
        default:          return libtokamap::DataType::Unknown;
    }
}

// Helper function to convert C++ TypedDataArray to Rust format
RustTypedDataArray convert_typed_data_array(libtokamap::TypedDataArray& cpp_array)
{
    RustTypedDataArray rust_array{};

    // Convert data type
    rust_array.data_type = static_cast<uint8_t>(cpp_array.data_type());

    // Copy size and shape
    rust_array.size = cpp_array.size();
    std::copy(cpp_array.shape().begin(), cpp_array.shape().end(), std::back_inserter(rust_array.shape));

    // Copy raw data
    size_t element_size = cpp_array.element_size();
    size_t total_bytes = rust_array.size * element_size;
    std::copy(cpp_array.buffer(), cpp_array.buffer() + total_bytes, std::back_inserter(rust_array.data));

    return rust_array;
}

// Helper function to convert Rust factory args to C++ format
libtokamap::DataSourceFactoryArgs convert_factory_args(const RustDataSourceFactoryArgs& rust_args)
{
    libtokamap::DataSourceFactoryArgs cpp_args;

    try {
        nlohmann::json json_args = nlohmann::json::parse(rust_args.json_string);

        // Convert JSON values to std::any
        for (const auto& [key, value] : json_args.items()) {
            if (value.is_string()) {
                cpp_args[key] = value.get<std::string>();
            } else if (value.is_number_integer()) {
                cpp_args[key] = value.get<int64_t>();
            } else if (value.is_number_unsigned()) {
                cpp_args[key] = value.get<uint64_t>();
            } else if (value.is_number_float()) {
                cpp_args[key] = value.get<double>();
            } else if (value.is_boolean()) {
                cpp_args[key] = value.get<bool>();
            } else {
                // For complex types, store as JSON string
                cpp_args[key] = value.dump();
            }
        }
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error(std::string("Failed to parse factory args JSON: ") + e.what());
    }

    return cpp_args;
}
