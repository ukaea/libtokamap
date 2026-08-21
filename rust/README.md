# libtokamap-rust

Rust bindings for the libtokamap C++ library, providing safe and ergonomic access to tokamak data mapping functionality.

## Overview

libtokamap is a C++ library designed for mapping and processing tokamak experimental data. This Rust wrapper provides a safe, memory-safe interface to the core functionality while maintaining high performance.

## Features

- **Safe C++ Interop**: Uses the `cxx` crate for zero-cost, safe C++ integration
- **Memory Safety**: Automatic memory management with Rust's ownership system
- **Type Safety**: Strong typing for data arrays and configuration
- **Error Handling**: Comprehensive error types using `thiserror`
- **JSON Configuration**: Serde-based JSON configuration support
- **Data Source Management**: Register and manage multiple data sources
- **Custom Functions**: Load and use custom mapping functions
- **Thread Safety**: Safe to use across threads (when the underlying C++ library supports it)

## Installation

Add this to your `Cargo.toml`:

```toml
[dependencies]
libtokamap-rust = "0.1.0"
```

### Prerequisites

1. **libtokamap C++ library**: Must be built and installed
2. **C++20 compiler**: Required for the C++ components
3. **CMake**: For building the C++ library
4. **System dependencies**: As required by libtokamap (HDF5, MDS+, etc.)

### Building from Source

1. First, build the libtokamap C++ library:
   ```bash
   cd libtokamap
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   make install
   ```

2. Then build the Rust wrapper:
   ```bash
   cd rust
   cargo build --release
   ```

## Quick Start

```rust
use libtokamap_rust::{MappingHandler, DataType, TokaMapError};
use serde_json::json;

fn main() -> Result<(), TokaMapError> {
    // Create a new mapping handler
    let mut handler = MappingHandler::new();
    
    // Initialize with JSON configuration
    let config = json!({
        "version": "1.0",
        "experiments": {
            "ITER": {
                "mapping_dir": "/data/iter/mappings",
                "groups": ["magnetics", "thomson", "ece"]
            }
        },
        "data_sources": {}
    });
    
    handler.init_with_json(&config)?;
    
    // Map some data
    let result = handler.map_data(
        "ITER",                           // experiment
        "/magnetics/bpol_probe_01",      // signal path
        DataType::Float,                  // expected data type
        1,                               // rank (1D array)
        Some(&json!({"shot": 12345, "time": 5.0}))  // extra attributes
    )?;
    
    // Work with the result
    println!("Data type: {:?}", result.data_type());
    println!("Size: {}", result.size());
    println!("Shape: {:?}", result.shape());
    
    // Convert to Rust vector
    let float_data: Vec<f32> = result.to_vec()?;
    println!("First value: {}", float_data[0]);
    
    Ok(())
}
```

## API Reference

### MappingHandler

The main interface for data mapping operations.

```rust
// Create a new handler
let mut handler = MappingHandler::new();

// Initialize from file
handler.init_with_path("config.json")?;

// Initialize from JSON value
handler.init_with_json(&config_json)?;

// Map data
let result = handler.map_data(experiment, path, data_type, rank, attributes)?;

// Reset handler
handler.reset()?;
```

### Data Source Management

```rust
// Register a data source factory from a shared library
handler.register_data_source_factory_from_lib(
    "hdf5_factory", 
    "/path/to/hdf5_datasource.so"
)?;

// Register a data source instance
let factory_args = [
    ("file_path".to_string(), json!("/data/experiment.h5")),
    ("dataset_prefix".to_string(), json!("signals"))
].into_iter().collect();

handler.register_data_source_with_factory(
    "my_hdf5_source",
    "hdf5_factory", 
    &factory_args
)?;

// Unregister when done
handler.unregister_data_source("my_hdf5_source")?;
```

### Custom Functions

```rust
// Load custom function library
handler.load_custom_function_library("/path/to/custom_functions.so")?;

// Unregister specific function
handler.unregister_custom_function("my_lib", "my_function")?;
```

### TypedDataArray

Represents typed arrays returned from mapping operations.

```rust
// Get metadata
let data_type = array.data_type();
let size = array.size();
let shape = array.shape();
let rank = array.rank();

// Convert to Rust types
let float_vec: Vec<f32> = array.to_vec()?;
let double_vec: Vec<f64> = array.to_vec()?;

// Access raw bytes
let bytes = array.as_bytes();

// Check if empty
if array.is_empty() {
    println!("No data");
}
```

## Data Types

The library supports these data types:

- `DataType::Char` - `i8`
- `DataType::UChar` - `u8`
- `DataType::Short` - `i16`
- `DataType::UShort` - `u16`
- `DataType::Int` - `i32`
- `DataType::UInt` - `u32`
- `DataType::Long` - `i64` (platform dependent)
- `DataType::ULong` - `u64` (platform dependent)
- `DataType::Int64` - `i64`
- `DataType::UInt64` - `u64`
- `DataType::Float` - `f32`
- `DataType::Double` - `f64`

## Error Handling

The library provides comprehensive error types:

```rust
use libtokamap_rust::TokaMapError;

match handler.init_with_path("config.json") {
    Ok(_) => println!("Success!"),
    Err(TokaMapError::Configuration(msg)) => eprintln!("Config error: {}", msg),
    Err(TokaMapError::DataType(msg)) => eprintln!("Data type error: {}", msg),
    Err(TokaMapError::Processing(msg)) => eprintln!("Processing error: {}", msg),
    Err(TokaMapError::Parameter(msg)) => eprintln!("Parameter error: {}", msg),
    Err(TokaMapError::DataSource(msg)) => eprintln!("Data source error: {}", msg),
    Err(TokaMapError::Generic(msg)) => eprintln!("Generic error: {}", msg),
}
```

## Configuration Format

The configuration uses JSON format compatible with the C++ library:

```json
{
  "version": "1.0",
  "dd_version": "3.0",
  "mapping_directory": "/data/mappings",
  "cache": {
    "enabled": true,
    "max_size_mb": 1024
  },
  "experiments": {
    "ITER": {
      "mapping_dir": "/data/iter/mappings",
      "groups": ["magnetics", "thomson", "ece"],
      "partition": [
        {
          "attribute": "shot",
          "selector": "exact"
        },
        {
          "attribute": "time",
          "selector": "closest"
        }
      ]
    }
  },
  "data_sources": {
    "mdsplus_factory": {
      "type": "library",
      "path": "/usr/local/lib/libmdsplus_datasource.so"
    }
  },
  "global_settings": {
    "trace_enabled": false,
    "default_timeout": 30
  }
}
```

## Examples

Check the `examples` module for comprehensive usage examples:

```rust
use libtokamap_rust::examples;

// Run all examples
examples::run_all_examples();

// Or run specific examples
examples::basic_usage_example()?;
examples::data_source_registration_example()?;
examples::custom_functions_example()?;
```

## Thread Safety

The `MappingHandler` is not `Send` or `Sync` by default due to the underlying C++ implementation. For multi-threaded usage, create separate handler instances per thread or use appropriate synchronization.

## Performance Considerations

- **Zero-copy Data Transfer**: Where possible, data is transferred without copying
- **Memory Ownership**: The Rust wrapper takes ownership of data from C++
- **Caching**: Utilizes the C++ library's built-in caching mechanisms
- **Batch Operations**: Process multiple mappings efficiently

## Troubleshooting

### Common Issues

1. **Library not found**: Ensure libtokamap is properly installed and in library path
2. **C++ version**: Requires C++20 compatible compiler
3. **Missing dependencies**: Install HDF5, MDS+, and other required libraries

### Debug Build

For debugging, use the debug build which includes additional error information:

```bash
cargo build  # Debug build
RUST_LOG=debug cargo test  # Run tests with logging
```

### CMake Integration

The build system automatically detects the libtokamap installation. If needed, you can specify paths:

```bash
export LIBTOKAMAP_ROOT=/path/to/libtokamap/install
cargo build
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Run the test suite: `cargo test`
6. Submit a pull request

### Development Setup

```bash
# Clone and build libtokamap
git clone https://github.com/your-org/libtokamap.git
cd libtokamap
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=ON
make -j$(nproc)

# Build Rust wrapper
cd ../rust
cargo build
cargo test
```

## License

This project is licensed under the MIT OR Apache-2.0 license - see the LICENSE files for details.

## Changelog

### 0.1.0 (TBD)
- Initial release
- Basic MappingHandler wrapper
- TypedDataArray support
- Data source management
- Custom function loading
- Comprehensive error handling
- Documentation and examples