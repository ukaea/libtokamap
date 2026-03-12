# LibTokaMap Documentation

LibTokaMap is a C++20 library for flexible data mapping and transformation with support for multiple data sources and mapping types.

## What is LibTokaMap?

LibTokaMap is a library that provides data transformation and mapping capabilities using [TokaMap](https://github.com/ukaea/tokamap) compatible mappings. It provides a plugin-based architecture for defining data sources and custom functions that can be called via the TokaMap `DATA_SOURCE` and `CUSTOM` mapping types.

### Key Features

- **Multiple Mapping Types**: Value, data source, expression, dimension, and custom mappings
- **Pluggable Data Sources**: Extensible architecture for custom data source implementations
- **JSON Configuration**: Human-readable JSON-based mapping definitions
- **Caching Support**: Built-in RAM caching for improved performance
- **Type Safety**: Strong typing with C++20 features
- **Subset Operations**: Advanced data slicing and subsetting capabilities
- **Scale/Offset Transformations**: Built-in support for linear data transformations

## Quick Start

An example configuration file:

```toml
mapping_directory = "/path/to/mappings"
schemas_directory = "/path/to/tokamap/schemas"
trace_enabled = true
cache_enabled = true
custom_function_libraries = [
    "custom_function_library.so"
]

[data_source_factories]
data_source_factory_name = "data_source_library.so"

[data_sources.JSON]
factory = "data_source_factory_name"
args.factory_argument_name = "factory constructor argument"
```

```cpp
#include <libtokamap.hpp>

int main() {
    libtokamap::MappingHandler mapping_handler;

    // Initialize with configuration
    mapping_handler.init("/path/to/config.toml");

    // Perform mapping
    libtokamap::DataType data_type = libtokamap::DataType::Double;
    int rank = 1;
    nlohmann::json extra_attributes = {{"shot", 12345}};

    auto result = mapping_handler.map(
        "my_mapping",   // experiment name
        "path/to/data", // mapping name
        data_type,
        rank,
        extra_attributes
    );

    return 0;
}
```

## Documentation Structure

### Getting Started
- [**Using the Library**](using-the-library.md) - Complete guide to integrating and using LibTokaMap in your projects

### Extending LibTokaMap
- [**Creating a New Data Source**](creating-data-source.md) - How to implement custom data sources
- [**Creating Custom Function Libraries**](creating-custom-functions.md) - How to create and register custom mapping functions

## Core Concepts

### Mapping Types

LibTokaMap supports several types of mappings:

- **Value Mapping**: Direct value assignment from JSON configuration
- **Data Source Mapping**: Retrieves data from registered data sources
- **Expression Mapping**: Evaluates mathematical expressions
- **Dimension Mapping**: Handles array dimension information
- **Custom Mapping**: User-defined mapping logic through custom libraries

### Data Sources

Data sources are pluggable components that retrieve data from various backends such as:

- JSON files
- Databases
- Network APIs
- Custom data formats

### Configuration

LibTokaMap uses a hierarchical configuration system based on JSON files:

```
mappings/
├── experiment1/
│   ├── mappings.cfg.json
│   ├── globals.json
│   └── group_name1/
│       └── partition1_0/
│           ├── globals.json
│           └── mappings.json
```

## Building the Library

### Requirements

- C++20 compatible compiler
- CMake 3.15+

### Bundled Libraries

The following libraries are bundled with LibTokaMap in the ext_include directory:

- CTRE (compile time regex) library
- nlohmann/json library
- Pantor/Inja templating engine
- ExprTk expression parsing library
- spdlog

### Build Instructions

```bash
cmake -Bbuild -DENABLE_TESTING=ON -DENABLE_EXAMPLES=ON
cmake --build build
```

### CMake Options

- `ENABLE_TESTING`: Build unit tests (default: OFF)
- `ENABLE_EXAMPLES`: Build example applications (default: OFF)
- `ENABLE_PROFILING`: Enable code profiling (default: OFF)
- `ENABLE_COVERAGE`: Enable test coverage (default: OFF)

## Examples

The `examples/` directory contains working examples:

- `simple_mapper/`: Basic mapping example with JSON data source
