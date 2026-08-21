//! Integration tests for the libtokamap Rust wrapper
//!
//! These tests verify that the Rust wrapper correctly interfaces with
//! the underlying C++ libtokamap library.

use libtokamap_rust::{DataType, MappingHandler, TokaMapError};
use serde_json::json;

#[test]
fn test_mapping_handler_creation() {
    let handler = MappingHandler::new();
    // Just verify that we can create a handler without panicking
    drop(handler);
}

#[test]
fn test_handler_reset() {
    let mut handler = MappingHandler::new();

    // Reset should work even on uninitialized handler
    match handler.reset() {
        Ok(_) => {}
        Err(_) => {
            // Reset might fail if handler isn't initialized, which is acceptable
        }
    }
}

#[test]
fn test_json_config_initialization() {
    let mut handler = MappingHandler::new();

    let config = json!({
        "version": "1.0",
        "experiments": {},
        "data_sources": {}
    });

    // This might fail due to missing actual mapping files, but should not panic
    let result = handler.init_with_json(&config);

    // We don't assert success because we may not have real mapping data
    // but we verify the call doesn't panic and returns a proper Result
    match result {
        Ok(_) => println!("Initialization succeeded"),
        Err(TokaMapError::Configuration(_)) => println!("Expected configuration error"),
        Err(e) => println!("Initialization failed with: {}", e),
    }
}

#[test]
fn test_invalid_json_config() {
    let mut handler = MappingHandler::new();

    // Test with completely invalid config
    let invalid_config = json!({
        "invalid_field": "invalid_value"
    });

    let result = handler.init_with_json(&invalid_config);
    assert!(result.is_err(), "Should fail with invalid configuration");

    if let Err(e) = result {
        assert!(matches!(e, TokaMapError::Configuration(_)));
    }
}

#[test]
fn test_path_initialization_with_nonexistent_file() {
    let mut handler = MappingHandler::new();

    // Test with non-existent file
    let result = handler.init_with_path("/nonexistent/path/config.json");
    assert!(result.is_err(), "Should fail with non-existent config file");

    if let Err(e) = result {
        assert!(matches!(e, TokaMapError::Configuration(_)));
    }
}

#[test]
fn test_data_source_operations() {
    let mut handler = MappingHandler::new();

    // Test registering a data source factory with non-existent library
    let result =
        handler.register_data_source_factory_from_lib("test_factory", "/nonexistent/library.so");

    // Should fail but not panic
    assert!(result.is_err());
    if let Err(e) = result {
        assert!(matches!(e, TokaMapError::DataSource(_)));
    }

    // Test unregistering non-existent data source
    let result = handler.unregister_data_source("nonexistent_source");

    // May succeed or fail depending on implementation, but shouldn't panic
    match result {
        Ok(_) => {}
        Err(TokaMapError::DataSource(_)) => {}
        Err(e) => panic!("Unexpected error type: {}", e),
    }
}

#[test]
fn test_custom_function_operations() {
    let mut handler = MappingHandler::new();

    // Test loading non-existent custom function library
    let result = handler.load_custom_function_library("/nonexistent/functions.so");

    // Should fail but not panic
    assert!(result.is_err());
    if let Err(e) = result {
        assert!(matches!(e, TokaMapError::Generic(_)));
    }

    // Test unregistering non-existent custom function
    let result = handler.unregister_custom_function("nonexistent_lib", "nonexistent_func");

    // May succeed or fail, but shouldn't panic
    match result {
        Ok(_) => {}
        Err(TokaMapError::Generic(_)) => {}
        Err(e) => panic!("Unexpected error type: {}", e),
    }
}

#[test]
fn test_data_mapping_without_initialization() {
    let mut handler = MappingHandler::new();

    // Try to map data without initialization
    let result = handler.map_data("test_experiment", "/test/path", DataType::Float, 1, None);

    // Should fail because handler is not initialized
    assert!(result.is_err());
}

#[test]
fn test_data_type_enum_values() {
    // Test that DataType enum values are as expected
    assert_eq!(DataType::Unknown as u32, 0);
    assert_eq!(DataType::Int8 as u32, 1);
    assert_eq!(DataType::Int16 as u32, 2);
    assert_eq!(DataType::Int32 as u32, 3);
    assert_eq!(DataType::Int64 as u32, 4);
    assert_eq!(DataType::UInt8 as u32, 5);
    assert_eq!(DataType::UInt16 as u32, 6);
    assert_eq!(DataType::UInt32 as u32, 7);
    assert_eq!(DataType::UInt64 as u32, 8);
    assert_eq!(DataType::Float as u32, 9);
    assert_eq!(DataType::Double as u32, 10);
}

#[test]
fn test_error_types() {
    // Test that error types can be created and match correctly
    let config_error = TokaMapError::Configuration("test".to_string());
    assert!(matches!(config_error, TokaMapError::Configuration(_)));

    let data_type_error = TokaMapError::DataType("test".to_string());
    assert!(matches!(data_type_error, TokaMapError::DataType(_)));

    let processing_error = TokaMapError::Processing("test".to_string());
    assert!(matches!(processing_error, TokaMapError::Processing(_)));

    let parameter_error = TokaMapError::Parameter("test".to_string());
    assert!(matches!(parameter_error, TokaMapError::Parameter(_)));

    let data_source_error = TokaMapError::DataSource("test".to_string());
    assert!(matches!(data_source_error, TokaMapError::DataSource(_)));

    let generic_error = TokaMapError::Generic("test".to_string());
    assert!(matches!(generic_error, TokaMapError::Generic(_)));
}

#[test]
fn test_factory_args_creation() {
    use libtokamap_rust::DataSourceFactoryArgs;

    let mut args = DataSourceFactoryArgs::new();
    args.insert("string_arg".to_string(), json!("test_value"));
    args.insert("int_arg".to_string(), json!(42));
    args.insert("float_arg".to_string(), json!(3.14));
    args.insert("bool_arg".to_string(), json!(true));

    assert_eq!(args.len(), 4);
    assert!(args.contains_key("string_arg"));
    assert!(args.contains_key("int_arg"));
    assert!(args.contains_key("float_arg"));
    assert!(args.contains_key("bool_arg"));
}

#[test]
fn test_multiple_handlers() {
    // Test that we can create multiple handlers
    let handler1 = MappingHandler::new();
    let handler2 = MappingHandler::new();
    let handler3 = MappingHandler::new();

    // All should be valid
    drop(handler1);
    drop(handler2);
    drop(handler3);
}

#[cfg(feature = "integration_with_real_data")]
#[test]
fn test_with_real_config() {
    // This test only runs if we have access to real configuration
    // Enable with: cargo test --features integration_with_real_data

    use std::env;

    let config_path = env::var("LIBTOKAMAP_TEST_CONFIG")
        .expect("LIBTOKAMAP_TEST_CONFIG environment variable not set");

    let mut handler = MappingHandler::new();
    let result = handler.init_with_path(&config_path);

    match result {
        Ok(_) => {
            println!("Successfully initialized with real config");

            // Try a real mapping operation
            if let Ok(mapped_data) = handler.map_data(
                "test_experiment",
                "/test/path",
                DataType::Float,
                1,
                Some(&json!({"test_attr": "test_value"})),
            ) {
                println!("Successfully mapped data: {} elements", mapped_data.size());
                println!("Data type: {:?}", mapped_data.data_type());
                println!("Shape: {:?}", mapped_data.shape());
            }
        }
        Err(e) => {
            println!("Failed to initialize with real config: {}", e);
        }
    }
}

// Benchmark tests (only run with --release)
#[cfg(test)]
mod benchmarks {
    use super::*;
    use std::time::Instant;

    #[test]
    #[ignore] // Run with: cargo test --release -- --ignored
    fn bench_handler_creation() {
        let start = Instant::now();
        let iterations = 1000;

        for _ in 0..iterations {
            let _handler = MappingHandler::new();
        }

        let duration = start.elapsed();
        println!("Created {} handlers in {:?}", iterations, duration);
        println!("Average time per creation: {:?}", duration / iterations);
    }

    #[test]
    #[ignore]
    fn bench_config_parsing() {
        let config = json!({
            "version": "1.0",
            "experiments": {
                "test1": {"mapping_dir": "/test1", "groups": ["g1", "g2"]},
                "test2": {"mapping_dir": "/test2", "groups": ["g3", "g4"]},
                "test3": {"mapping_dir": "/test3", "groups": ["g5", "g6"]},
            },
            "data_sources": {}
        });

        let start = Instant::now();
        let iterations = 100;

        for _ in 0..iterations {
            let mut handler = MappingHandler::new();
            let _ = handler.init_with_json(&config);
        }

        let duration = start.elapsed();
        println!("Parsed config {} times in {:?}", iterations, duration);
        println!("Average time per parse: {:?}", duration / iterations);
    }
}
