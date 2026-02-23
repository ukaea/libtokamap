# LibTokaMap

A C++20 library for flexible data mapping and transformation with support for multiple data sources and mapping types.

## Overview

LibTokaMap is a flexible mapping library designed to handle complex data transformations through configurable JSON-based mapping definitions. It provides a plugin-based architecture for data sources and supports various mapping types including value mappings, data source mappings, expression mappings, and custom mappings.

## Features

- **Multiple Mapping Types**: Support for value, data source, expression, dimension, and custom mappings
- **Pluggable Data Sources**: Extensible architecture for custom data source implementations
- **JSON Configuration**: Human-readable JSON-based mapping definitions
- **Caching Support**: Built-in RAM caching for improved performance
- **Type Safety**: Strong typing with C++20 features
- **Subset Operations**: Advanced data slicing and subsetting capabilities
- **Scale/Offset Transformations**: Built-in support for linear data transformations

## Building

### Requirements

- C++20 compatible compiler
- CMake 3.15+
- nlohmann/json library
- Pantor/Inja templating engine
- GSL-lite (Guidelines Support Library)
- ExprTk expression parsing library

### Build Instructions

```bash
mkdir build
cd build
cmake .. -DENABLE_TESTING=ON -DENABLE_EXAMPLES=ON
make
```

### CMake Options

- `ENABLE_TESTING`: Build unit tests (default: OFF)
- `ENABLE_EXAMPLES`: Build example applications (default: OFF)

## Usage

### Basic Example

```cpp
#include <libtokamap.hpp>

int main() {
    libtokamap::MappingHandler mapping_handler;

    // Initialize with configuration
    nlohmann::json config = {
        {"mapping_directory", "/path/to/mappings"}
    };
    mapping_handler.init(config);

    // Perform mapping
    std::type_index data_type = std::type_index{typeid(double)};
    int rank = 1;
    nlohmann::json extra_attributes = {{"shot", 12345}};

    auto result = mapping_handler.map(
        "my_mapping",
        "path/to/data",
        data_type,
        rank,
        extra_attributes
    );

    return 0;
}
```

### Custom Data Source

```cpp
class MyDataSource : public libtokamap::DataSource {
public:
    TypedDataArray get(const DataSourceArgs& map_args,
                      const MapArguments& arguments,
                      RamCache* ram_cache) override {
        // Implement your data retrieval logic
        // Return TypedDataArray with your data
    }
};

// Register the data source
auto data_source = std::make_unique<MyDataSource>();
libtokamap::DataSourceMapping::register_data_source("MY_SOURCE", std::move(data_source));
```

## Mapping Types

### Value Mapping
Direct value assignment from JSON configuration.

```json
{
    "map_type": "VALUE",
    "value": 42
}
```

### Data Source Mapping
Retrieves data from registered data sources.

```json
{
    "map_type": "DATA_SOURCE",
    "data_source": "JSON",
    "args": {
        "file": "data.json",
        "path": "measurements.temperature"
    },
    "scale": 1.0,
    "offset": 0.0,
    "slice": "[0:10]"
}
```

### Expression Mapping
Evaluates mathematical expressions.

```json
{
    "map_type": "EXPR",
    "expr": "x * 2 + y",
    "parameters": {
        "x": "path/to/x",
        "y": "path/to/y"
    }
}
```

### Dimension Mapping
Handles array dimension information.

```json
{
    "map_type": "DIMENSION",
    "dim_probe": "array/path"
}
```

### Custom Mapping
User-defined mapping logic.

```json
{
    "map_type": "CUSTOM",
    "custom_type": "my_custom_mapping"
}
```

## Configuration

### Mapping Directory Structure

```
mappings/
├── experiment1/
│   ├── mappings.cfg.json
│   ├── globals.json
│   └── group_name1/
│       └── partition1_0/
│           ├── globals.json
│           └── mappings.json
│       └── partition1_100/
│           ├── globals.json
│           └── mappings.json
│       └── ...
│   └── group_name2/
│       └── ...
└── experiment2/
    └── ...
```

### Configuration File Format

```json
{
    "mapping_directory": "/path/to/mappings",
    "cache_enabled": true,
    "cache_size": 100
}
```

## API Reference

### MappingHandler

Main class for handling mapping operations.

#### Methods

- `void init(const nlohmann::json& config)`: Initialize with configuration
- `TypedDataArray map(...)`: Perform mapping operation
- `void reset()`: Reset handler state

### DataSource

Base class for implementing custom data sources.

#### Virtual Methods

- `TypedDataArray get(...)`: Retrieve data from source

### Mapping Types

Base classes for different mapping implementations:

- `Mapping`: Base mapping interface
- `ValueMapping`: Direct value mappings
- `DataSourceMapping`: Data source mappings
- `ExprMapping`: Expression-based mappings
- `DimMapping`: Dimension mappings
- `CustomMapping`: User-defined mappings

## Utilities

### Subset Operations
- Slice parsing: `"[0:10]"`, `"[::2]"`, `"[1:5:2]"`
- Multi-dimensional slicing support
- Range validation and boundary checking

### Scale/Offset Transformations
- Linear transformations: `output = input * scale + offset`
- Applied after data retrieval and slicing

### Caching
- RAM-based caching for frequently accessed data
- Configurable cache policies
- Automatic cache invalidation

## Examples

See the `examples/` directory for complete working examples:

- `simple_mapper/`: Basic mapping example with JSON data source

## Testing

Run the test suite:

```bash
cd build
make test
```

## Version Information

Current version: 0.1.0

## Contributing

### Code Style
- Follow C++20 best practices
- Use clang-format for code formatting
- Enable all compiler warnings (`-Wall -Werror -Wpedantic`)

### TODO Items
- [ ] Make mapping values case insensitive
- [x] ~~Remove references to IDSs~~
- [x] ~~Replace reinterpret_cast with bit_cast~~ (still exist in map_arguments.hpp)
- [x] ~~Replace boost::split with std::views::split~~
- [x] ~~Switch from using std::type_index to DataType enum~~
- [x] ~~Add exception types~~
- [x] ~~Add README and docs for library~~
- [ ] Tidy up DataSource get(...) arguments
- [x] ~~Adding system packaging to CMake~~
- [x] ~~Make mapping directory nesting configurable~~
- [x] ~~Remove SignalType logic~~
- [ ] Add C++20 template constraints
- [x] ~~Replace std::string{} with string_literals~~
- [ ] Fix logging
- [x] ~~Add tests for parse_slices~~
- [x] ~~Replace gsl::span with std::span~~
- [ ] Handle mismatch of request data type and returned data type, i.e. type conversions?
- [ ] Check returned data against expected rank
- [x] ~~Add JSON schema files into repo (from IMAS MASTU mapping)~~
- [x] ~~Validate JSON mappings on read~~
- [ ] Use std::format instead of string concatenation

## License

See the main project LICENSE file for license information.

## Dependencies

- **nlohmann/json**: JSON parsing and manipulation
- **Pantor/Inja**: Template engine for dynamic content generation
- **ExprTk**: Mathematical expression parsing and evaluation
