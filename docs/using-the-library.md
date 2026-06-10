# Using the Library

This guide covers everything you need to know to integrate and use LibTokaMap in your C++ projects.

## Installation

### Building from Source

First, clone the repository and build the library:

```bash
git clone <repository-url>
cd libtokamap
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
make install  # Optional: install system-wide
```

### CMake Integration

To use LibTokaMap in your CMake project, add it as a dependency:

```cmake
find_package(libtokamap REQUIRED)
target_link_libraries(your_target libtokamap::libtokamap)
```

Or if building as a subdirectory:

```cmake
add_subdirectory(libtokamap)
target_link_libraries(your_target libtokamap)
```

## Basic Usage

### Include Headers

```cpp
#include <libtokamap.hpp>
```

### Initialize the Mapping Handler

The `MappingHandler` is the main entry point for using LibTokaMap:

```cpp
#include <libtokamap.hpp>
#include <nlohmann/json.hpp>

int main() {
    // Create a mapping handler
    libtokamap::MappingHandler mapping_handler;
    
    // Configure the handler
    nlohmann::json config = {
        {"mapping_directory", "/path/to/your/mappings"},
        {"cache_enabled", true},
        {"cache_size", 100}
    };
    
    // Initialize with configuration
    mapping_handler.init(config);
    
    return 0;
}
```

### Performing Mappings

Once initialized, you can perform mappings using the `map()` method:

```cpp
// Define mapping parameters
std::string mapping_name = "temperature_data";
std::string data_path = "sensors/temperature";
std::type_index data_type = std::type_index{typeid(double)};
int rank = 1;  // 1D array
nlohmann::json extra_attributes = {
    {"shot", 12345},
    {"timestamp", "2024-01-15T10:30:00Z"}
};

// Perform the mapping
try {
    auto result = mapping_handler.map(
        mapping_name,
        data_path,
        data_type,
        rank,
        extra_attributes
    );
    
    // Use the result
    if (result.holds_alternative<std::vector<double>>()) {
        auto data = std::get<std::vector<double>>(result);
        // Process your data...
    }
} catch (const libtokamap::TokaMapError& e) {
    std::cerr << "Mapping error: " << e.what() << std::endl;
}
```

## Configuration

### Directory Structure

LibTokaMap uses a hierarchical directory structure for organizing mappings:

```
/path/to/mappings/
├── experiment1/
│   ├── mappings.cfg.json      # Experiment configuration
│   ├── globals.json           # Global variables for experiment
│   └── group_name1/
│       ├── partition1_0/
│       │   ├── globals.json   # Partition-specific globals
│       │   └── mappings.json  # Mapping definitions
│       └── partition1_100/
│           ├── globals.json
│           └── mappings.json
└── experiment2/
    └── ...
```

### Configuration File Format

The main configuration passed to `init()` supports these options:

```json
{
    "mapping_directory": "/path/to/mappings",
    "cache_enabled": true,
    "cache_size": 100,
    "max_nesting_depth": 5
}
```

#### Configuration Options

- `mapping_directory`: Root directory containing mapping definitions
- `cache_enabled`: Enable/disable RAM caching (default: true)
- `cache_size`: Maximum number of cached items (default: 100)
- `max_nesting_depth`: Maximum directory nesting depth (default: 5)

### Mapping Configuration Files

#### mappings.cfg.json

Defines the structure and available partitions for an experiment:

```json
{
    "name": "experiment1",
    "groups": {
        "group_name1": {
            "partitions": [
                {"name": "partition1_0", "range": [0, 99]},
                {"name": "partition1_100", "range": [100, 199]}
            ]
        }
    }
}
```

#### mappings.json

Contains the actual mapping definitions:

```json
{
    "temperature_data": {
        "map_type": "DATA_SOURCE",
        "data_source": "JSON",
        "args": {
            "file": "sensor_data.json",
            "path": "measurements.temperature"
        },
        "scale": 1.0,
        "offset": 273.15,
        "slice": "[0:100]"
    },
    "pressure_calculation": {
        "map_type": "EXPR",
        "expr": "temp * 0.1 + offset",
        "parameters": {
            "temp": "temperature_data",
            "offset": 1013.25
        }
    }
}
```

#### globals.json

Defines variables available to all mappings in the scope:

```json
{
    "shot_number": 12345,
    "calibration_factor": 1.05,
    "data_root": "/data/experiment1"
}
```

## Mapping Types

### Value Mapping

Direct assignment of values from configuration:

```json
{
    "constant_value": {
        "map_type": "VALUE",
        "value": 42.0
    },
    "array_value": {
        "map_type": "VALUE",
        "value": [1.0, 2.0, 3.0, 4.0]
    }
}
```

### Data Source Mapping

Retrieve data from registered data sources:

```json
{
    "sensor_data": {
        "map_type": "DATA_SOURCE",
        "data_source": "JSON",
        "args": {
            "file": "data/sensors.json",
            "path": "temperature.readings"
        },
        "scale": 1.0,
        "offset": 0.0,
        "slice": "[10:50:2]"
    }
}
```

#### Scale and Offset

Apply linear transformation: `output = input * scale + offset`

```json
{
    "temperature_celsius": {
        "map_type": "DATA_SOURCE",
        "data_source": "JSON",
        "args": {
            "file": "temp_kelvin.json",
            "path": "temperature"
        },
        "scale": 1.0,
        "offset": -273.15
    }
}
```

#### Slicing

Extract subsets of data using slice notation:

```json
{
    "data_subset": {
        "map_type": "DATA_SOURCE",
        "data_source": "JSON",
        "args": {"file": "data.json", "path": "measurements"},
        "slice": "[0:100:2]"  // Elements 0, 2, 4, ..., 98
    }
}
```

Slice formats:
- `"[start:end]"` - Elements from start to end-1
- `"[start:end:step]"` - Elements from start to end-1 with step
- `"[::step]"` - Every step-th element
- `"[start:]"` - From start to end
- `"[:end]"` - From beginning to end-1

### Expression Mapping

Evaluate mathematical expressions using ExprTk:

```json
{
    "calculated_pressure": {
        "map_type": "EXPR",
        "expr": "sqrt(x^2 + y^2) * scale_factor",
        "parameters": {
            "x": "sensor_x_data",
            "y": "sensor_y_data",
            "scale_factor": 1.05
        }
    }
}
```

Supported mathematical functions:
- Basic operations: `+`, `-`, `*`, `/`, `^`
- Functions: `sin`, `cos`, `tan`, `sqrt`, `log`, `exp`, `abs`
- Constants: `pi`, `e`

### Dimension Mapping

Get dimension information from arrays:

```json
{
    "array_size": {
        "map_type": "DIMENSION",
        "dim_probe": "large_data_array"
    }
}
```

### Custom Mapping

Use custom functions from loaded libraries:

```json
{
    "custom_transform": {
        "map_type": "CUSTOM",
        "custom_type": "my_custom_function",
        "library": "my_library",
        "inputs": {
            "input1": "source_data_1",
            "input2": "source_data_2"
        },
        "params": {
            "threshold": 0.5,
            "method": "linear"
        }
    }
}
```

## Working with Data Types

### TypedDataArray

LibTokaMap uses `TypedDataArray` to handle different data types:

```cpp
// Check the type of returned data
if (result.holds_alternative<std::vector<double>>()) {
    auto double_data = std::get<std::vector<double>>(result);
    // Process double data...
} else if (result.holds_alternative<std::vector<int>>()) {
    auto int_data = std::get<std::vector<int>>(result);
    // Process int data...
} else if (result.holds_alternative<std::vector<float>>()) {
    auto float_data = std::get<std::vector<float>>(result);
    // Process float data...
}
```

### Supported Types

- `std::vector<double>`
- `std::vector<float>`
- `std::vector<int>`
- `std::vector<std::string>`
- Single values of the above types

### Type Specification

When calling `map()`, specify the expected data type:

```cpp
// For double arrays
std::type_index data_type = std::type_index{typeid(double)};

// For integer arrays  
std::type_index data_type = std::type_index{typeid(int)};

// For string arrays
std::type_index data_type = std::type_index{typeid(std::string)};
```

## Caching

### RAM Cache

LibTokaMap includes built-in RAM caching to improve performance:

```cpp
nlohmann::json config = {
    {"mapping_directory", "/path/to/mappings"},
    {"cache_enabled", true},
    {"mapping_cache_enabled", true},
    {"data_source_cache_enabled", true},
    {"ram_cache_enabled", true},
    {"cache_size", 200}  // Cache up to 200 items
};
```

### Cache Behavior

- `cache_enabled = false` disables all cache layers
- `mapping_cache_enabled` controls complete mapping-result caching
- `data_source_cache_enabled` controls raw data-source fetch caching before slice/scale/offset
- `ram_cache_enabled` controls the `RamCache` pointer passed to data sources
- `RamCache` entries are automatically managed with LRU eviction
- Cache can be disabled by setting `cache_enabled` to `false`
- Cache size can be adjusted with `cache_size` parameter

## Error Handling

### Exception Types

LibTokaMap defines several exception types:

```cpp
try {
    auto result = mapping_handler.map(...);
} catch (const libtokamap::FileError& e) {
    // File not found or access issues
    std::cerr << "File error: " << e.what() << std::endl;
} catch (const libtokamap::DataSourceError& e) {
    // Data source specific errors
    std::cerr << "Data source error: " << e.what() << std::endl;
} catch (const libtokamap::MappingError& e) {
    // Mapping configuration or execution errors
    std::cerr << "Mapping error: " << e.what() << std::endl;
} catch (const libtokamap::TokaMapError& e) {
    // Base class for all LibTokaMap errors
    std::cerr << "LibTokaMap error: " << e.what() << std::endl;
}
```

### Common Error Scenarios

1. **Missing mapping definition**: Ensure mapping exists in configuration
2. **Invalid slice format**: Check slice syntax in mapping definition
3. **Data source not found**: Verify data source is registered
4. **File not found**: Check file paths in data source arguments
5. **Type mismatch**: Ensure requested type matches returned data

## Best Practices

### Performance Tips

1. **Use caching**: Enable RAM cache for frequently accessed data
2. **Optimize slicing**: Use slicing to reduce data transfer when possible
3. **Batch operations**: Group related mappings in single configuration files
4. **Minimize file I/O**: Structure data to minimize file access

### Configuration Organization

1. **Use globals**: Define common values in `globals.json` files
2. **Logical grouping**: Organize mappings by functionality or data source
3. **Consistent naming**: Use descriptive, consistent names for mappings
4. **Documentation**: Comment complex expressions and mappings

### Error Handling

1. **Validate inputs**: Check mapping names and parameters before calling `map()`
2. **Handle exceptions**: Always wrap mapping calls in try-catch blocks
3. **Log errors**: Implement proper error logging for debugging
4. **Graceful degradation**: Provide fallback behavior for failed mappings

## Complete Example

Here's a complete example demonstrating typical usage:

```cpp
#include <iostream>
#include <libtokamap.hpp>
#include <nlohmann/json.hpp>

int main() {
    try {
        // Initialize mapping handler
        libtokamap::MappingHandler handler;
        
        nlohmann::json config = {
            {"mapping_directory", "/data/mappings"},
            {"cache_enabled", true},
            {"cache_size", 50}
        };
        
        handler.init(config);
        
        // Prepare mapping arguments
        std::type_index data_type = std::type_index{typeid(double)};
        int rank = 1;
        nlohmann::json attributes = {
            {"shot", 12345},
            {"diagnostic", "temperature"}
        };
        
        // Perform mapping
        auto result = handler.map(
            "temperature_calibrated",
            "sensors/temperature",
            data_type,
            rank,
            attributes
        );
        
        // Process results
        if (result.holds_alternative<std::vector<double>>()) {
            auto temp_data = std::get<std::vector<double>>(result);
            
            std::cout << "Retrieved " << temp_data.size() << " temperature readings\n";
            
            // Calculate statistics
            double sum = 0.0;
            for (double temp : temp_data) {
                sum += temp;
            }
            double average = sum / temp_data.size();
            
            std::cout << "Average temperature: " << average << " °C\n";
        }
        
    } catch (const libtokamap::TokaMapError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

This example demonstrates initialization, configuration, mapping execution, and result processing with proper error handling.
