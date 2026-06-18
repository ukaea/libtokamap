#pragma once

#include <cstddef>
#include <cstdint>
#include <libtokamap.hpp>
#include <memory>
#include <rust/cxx.h>

// Rust-compatible types (defined to match the Rust bridge)
struct RustTypedDataArray;
struct RustDataSourceFactoryArgs;

// C++ wrapper class for MappingHandler to provide a clean interface for Rust
class MappingHandlerWrapper
{
  private:
    libtokamap::MappingHandler handler;

  public:
    MappingHandlerWrapper() = default;
    ~MappingHandlerWrapper() = default;

    // Disable copy and move to keep it simple
    MappingHandlerWrapper(const MappingHandlerWrapper&) = delete;
    MappingHandlerWrapper& operator=(const MappingHandlerWrapper&) = delete;
    MappingHandlerWrapper(MappingHandlerWrapper&&) = delete;
    MappingHandlerWrapper& operator=(MappingHandlerWrapper&&) = delete;

    // Reset the handler
    void reset();

    // Initialize with config file path
    void init_with_path(rust::Str config_path);

    // Initialize with JSON config string
    void init_with_json(rust::Str config_json);

    // Map data
    RustTypedDataArray map_data(rust::Str experiment, rust::Str path, uint32_t data_type_index, int32_t rank,
                                rust::Str extra_attributes);

    // Register data source factory with library path
    void register_data_source_factory_with_lib(rust::Str factory_name, rust::Str library_path);

    // Register data source with factory
    void register_data_source_with_factory(rust::Str name, rust::Str factory_name,
                                           const RustDataSourceFactoryArgs& args);

    // Unregister data source
    void unregister_data_source(rust::Str name);

    // Load custom function library
    void load_custom_function_library(rust::Str library_path);

    // Unregister custom function
    void unregister_custom_function(rust::Str library_name, rust::Str function_name);
};

// Factory function to create new MappingHandlerWrapper
std::unique_ptr<MappingHandlerWrapper> new_mapping_handler();

// Helper functions for type conversion
libtokamap::DataType data_type_index_from_u32(uint32_t data_type);
RustTypedDataArray convert_typed_data_array(libtokamap::TypedDataArray& cpp_array);
libtokamap::DataSourceFactoryArgs convert_factory_args(const RustDataSourceFactoryArgs& rust_args);
