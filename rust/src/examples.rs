//! Examples of using the libtokamap Rust wrapper
//!
//! This module provides examples of how to use the MappingHandler
//! and other components of the libtokamap Rust wrapper.

use crate::{DataSourceFactoryArgs, MappingHandler, TokaMapError};
use serde_json::{json, Value};

/// Example of basic MappingHandler usage
pub fn basic_usage_example() -> Result<(), TokaMapError> {
    // Create a new mapping handler
    let mut handler = MappingHandler::new();

    // Initialize with a configuration file
    // handler.init_with_path("path/to/config.json")?;

    // Or initialize with a JSON configuration
    let config = json!({
        "version": "1.0",
        "experiments": {
            "test_experiment": {
                "mapping_dir": "/path/to/mappings",
                "groups": ["group1", "group2"]
            }
        },
        "data_sources": {}
    });

    handler.init_with_json(&config)?;

    // Map some data (this would normally work with real data)
    // let result = handler.map_data(
    //     "test_experiment",
    //     "/some/data/path",
    //     DataType::Float,
    //     2,  // rank
    //     Some(&json!({"time": 1000.0}))
    // )?;

    println!("MappingHandler initialized successfully");
    Ok(())
}

/// Example of registering data sources
pub fn data_source_registration_example() -> Result<(), TokaMapError> {
    let mut handler = MappingHandler::new();

    // Register a data source factory from a dynamic library
    handler
        .register_data_source_factory_from_lib("hdf5_factory", "/path/to/hdf5_data_source.so")?;

    // Create factory arguments for the data source
    let mut factory_args = DataSourceFactoryArgs::new();
    factory_args.insert("file_path".to_string(), json!("/data/experiment.h5"));
    factory_args.insert("dataset_prefix".to_string(), json!("signals"));

    // Register a data source using the factory
    handler.register_data_source_with_factory("hdf5_data_source", "hdf5_factory", &factory_args)?;

    println!("Data source registered successfully");

    // Later, you can unregister the data source if needed
    handler.unregister_data_source("hdf5_data_source")?;

    Ok(())
}

/// Example of loading and using custom functions
pub fn custom_functions_example() -> Result<(), TokaMapError> {
    let mut handler = MappingHandler::new();

    // Load a custom function library
    handler.load_custom_function_library("/path/to/custom_functions.so")?;

    println!("Custom function library loaded successfully");

    // Custom functions would be used automatically during mapping
    // based on the mapping configuration

    // You can also unregister specific custom functions
    handler.unregister_custom_function("my_library", "my_function")?;

    Ok(())
}

/// Example configuration for a tokamak experiment
pub fn tokamak_config_example() -> Value {
    json!({
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
            },
            "JET": {
                "mapping_dir": "/data/jet/mappings",
                "groups": ["magnetics", "bolometry", "cxrs"],
                "partition": [
                    {
                        "attribute": "pulse",
                        "selector": "exact"
                    }
                ]
            }
        },
        "data_sources": {
            "mdsplus_factory": {
                "type": "library",
                "path": "/usr/local/lib/libmdsplus_datasource.so"
            },
            "hdf5_factory": {
                "type": "library",
                "path": "/usr/local/lib/libhdf5_datasource.so"
            }
        },
        "global_settings": {
            "trace_enabled": false,
            "default_timeout": 30
        }
    })
}

/// Example of working with TypedDataArray results
pub fn typed_data_array_example() -> Result<(), TokaMapError> {
    let mut handler = MappingHandler::new();
    let config = tokamak_config_example();
    handler.init_with_json(&config)?;

    // This would normally return real data from a mapping operation
    // let result = handler.map_data(
    //     "ITER",
    //     "/magnetics/bpol_probe_01",
    //     DataType::Float,
    //     1,  // 1D array
    //     Some(&json!({"shot": 12345, "time": 5.0}))
    // )?;

    // Example of what you could do with the result:
    // println!("Data type: {:?}", result.data_type());
    // println!("Size: {}", result.size());
    // println!("Shape: {:?}", result.shape());
    // println!("Rank: {}", result.rank());

    // Convert to a Rust vector (if the types match)
    // let float_data: Vec<f32> = result.to_vec()?;
    // println!("First few values: {:?}", &float_data[..5.min(float_data.len())]);

    Ok(())
}

/// Example of error handling
pub fn error_handling_example() {
    let mut handler = MappingHandler::new();

    // Try to initialize with invalid JSON
    let invalid_config = json!({
        "version": "1.0",
        // Missing required fields
    });

    match handler.init_with_json(&invalid_config) {
        Ok(_) => println!("Initialization succeeded"),
        Err(TokaMapError::Configuration(msg)) => {
            println!("Configuration error: {}", msg);
        }
        Err(TokaMapError::DataType(msg)) => {
            println!("Data type error: {}", msg);
        }
        Err(TokaMapError::Processing(msg)) => {
            println!("Processing error: {}", msg);
        }
        Err(TokaMapError::Parameter(msg)) => {
            println!("Parameter error: {}", msg);
        }
        Err(TokaMapError::DataSource(msg)) => {
            println!("Data source error: {}", msg);
        }
        Err(TokaMapError::Generic(msg)) => {
            println!("Generic error: {}", msg);
        }
        Err(TokaMapError::Thread(msg)) => {
            println!("Thread error: {}", msg);
        }
    }
}

/// Run all examples (for testing)
pub fn run_all_examples() {
    println!("Running libtokamap Rust wrapper examples...\n");

    println!("1. Basic usage example:");
    if let Err(e) = basic_usage_example() {
        println!("   Error: {}", e);
    }

    println!("\n2. Data source registration example:");
    if let Err(e) = data_source_registration_example() {
        println!("   Error: {}", e);
    }

    println!("\n3. Custom functions example:");
    if let Err(e) = custom_functions_example() {
        println!("   Error: {}", e);
    }

    println!("\n4. TypedDataArray example:");
    if let Err(e) = typed_data_array_example() {
        println!("   Error: {}", e);
    }

    println!("\n5. Error handling example:");
    error_handling_example();

    println!("\n6. Example tokamak configuration:");
    let config = tokamak_config_example();
    println!("{}", serde_json::to_string_pretty(&config).unwrap());

    println!("\nExamples completed!");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_tokamak_config_generation() {
        let config = tokamak_config_example();
        assert!(config.is_object());
        assert_eq!(config["version"], "1.0");
        assert!(config["experiments"].is_object());
    }

    #[test]
    fn test_examples_dont_panic() {
        // These examples might fail due to missing files/configuration,
        // but they shouldn't panic
        let _ = basic_usage_example();
        let _ = data_source_registration_example();
        let _ = custom_functions_example();
        let _ = typed_data_array_example();
        error_handling_example(); // This one shouldn't fail
    }
}
