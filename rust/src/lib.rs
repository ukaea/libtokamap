//! Rust bindings for libtokamap C++ library
//!
//! This crate provides safe Rust bindings for the libtokamap C++ library,
//! which is used for tokamak data mapping and processing.

use serde_json::Value as JsonValue;
use std::collections::HashMap;
use thiserror::Error;

pub mod examples;

/// Error types for the libtokamap Rust wrapper
#[derive(Error, Debug)]
pub enum TokaMapError {
    #[error("Configuration error: {0}")]
    Configuration(String),
    #[error("Data type error: {0}")]
    DataType(String),
    #[error("Processing error: {0}")]
    Processing(String),
    #[error("Parameter error: {0}")]
    Parameter(String),
    #[error("Data source error: {0}")]
    DataSource(String),
    #[error("Generic error: {0}")]
    Generic(String),
    #[error("Thread error: {0}")]
    Thread(String),
}

/// Data types supported by TypedDataArray
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DataType {
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
}

/// A Rust representation of the C++ TypedDataArray
#[derive(Debug)]
pub struct TypedDataArray {
    data_type: DataType,
    size: usize,
    shape: Vec<usize>,
    data: Vec<u8>,
}

impl TypedDataArray {
    /// Get the data type of the array
    pub fn data_type(&self) -> DataType {
        self.data_type
    }

    /// Get the size (number of elements) of the array
    pub fn size(&self) -> usize {
        self.size
    }

    /// Get the shape of the array
    pub fn shape(&self) -> &[usize] {
        &self.shape
    }

    /// Get the rank (number of dimensions) of the array
    pub fn rank(&self) -> usize {
        self.shape.len()
    }

    /// Check if the array is empty
    pub fn is_empty(&self) -> bool {
        self.size == 0
    }

    /// Get the raw data as bytes
    pub fn as_bytes(&self) -> &[u8] {
        &self.data
    }

    /// Convert to a vector of the specified type (if compatible)
    pub fn to_vec<T: Copy + 'static>(&self) -> Result<Vec<T>, TokaMapError> {
        let expected_size = std::mem::size_of::<T>();
        let actual_element_size = self.data.len() / self.size;

        if expected_size != actual_element_size {
            return Err(TokaMapError::DataType(format!(
                "Type size mismatch: expected {}, got {}",
                expected_size, actual_element_size
            )));
        }

        let ptr = self.data.as_ptr() as *const T;
        let slice = unsafe { std::slice::from_raw_parts(ptr, self.size) };
        Ok(slice.to_vec())
    }
}

/// Arguments for data source factory functions
pub type DataSourceFactoryArgs = HashMap<String, JsonValue>;

#[cxx::bridge]
mod ffi {
    /// Rust types that will be shared with C++
    #[derive(Debug)]
    pub struct RustTypedDataArray {
        pub data_type: u8,
        pub size: usize,
        pub shape: Vec<usize>,
        pub data: Vec<u8>,
    }

    #[derive(Debug)]
    pub struct RustDataSourceFactoryArgs {
        pub json_string: String,
    }

    unsafe extern "C++" {
        include!("libtokamap-rust/src/bridge.hpp");

        /// C++ MappingHandler wrapper
        type MappingHandlerWrapper;

        /// Create a new MappingHandler instance
        fn new_mapping_handler() -> UniquePtr<MappingHandlerWrapper>;

        /// Reset the MappingHandler
        fn reset(self: Pin<&mut MappingHandlerWrapper>) -> Result<()>;

        /// Initialize with config file path
        fn init_with_path(self: Pin<&mut MappingHandlerWrapper>, config_path: &str) -> Result<()>;

        /// Initialize with JSON config string
        fn init_with_json(self: Pin<&mut MappingHandlerWrapper>, config_json: &str) -> Result<()>;

        /// Map data
        fn map_data(
            self: Pin<&mut MappingHandlerWrapper>,
            experiment: &str,
            path: &str,
            data_type_index: u32,
            rank: i32,
            extra_attributes: &str,
        ) -> Result<RustTypedDataArray>;

        /// Register data source factory with library path
        fn register_data_source_factory_with_lib(
            self: Pin<&mut MappingHandlerWrapper>,
            factory_name: &str,
            library_path: &str,
        ) -> Result<()>;

        /// Register data source with factory
        fn register_data_source_with_factory(
            self: Pin<&mut MappingHandlerWrapper>,
            name: &str,
            factory_name: &str,
            args: &RustDataSourceFactoryArgs,
        ) -> Result<()>;

        /// Unregister data source
        fn unregister_data_source(self: Pin<&mut MappingHandlerWrapper>, name: &str) -> Result<()>;

        /// Load custom function library
        fn load_custom_function_library(
            self: Pin<&mut MappingHandlerWrapper>,
            library_path: &str,
        ) -> Result<()>;

        /// Unregister custom function
        fn unregister_custom_function(
            self: Pin<&mut MappingHandlerWrapper>,
            library_name: &str,
            function_name: &str,
        ) -> Result<()>;
    }
}

/// Main Rust wrapper for the MappingHandler
pub struct MappingHandler {
    inner: std::sync::Mutex<cxx::UniquePtr<ffi::MappingHandlerWrapper>>,
}

unsafe impl Send for MappingHandler {}

impl MappingHandler {
    /// Create a new MappingHandler
    pub fn new() -> Self {
        Self {
            inner: std::sync::Mutex::new(ffi::new_mapping_handler()),
        }
    }

    /// Reset the handler
    pub fn reset(&mut self) -> Result<(), TokaMapError> {
        self.inner
            .lock()
            .map_err(|_| TokaMapError::Thread("mutex lock failed".to_string()))?
            .pin_mut()
            .reset()
            .map_err(|e| TokaMapError::Generic(format!("Reset failed: {}", e)))
    }

    /// Initialize with a configuration file
    pub fn init_with_path(
        &mut self,
        config_path: impl AsRef<std::path::Path>,
    ) -> Result<(), TokaMapError> {
        let path_str = config_path.as_ref().to_string_lossy();
        self.inner
            .lock()
            .map_err(|_| TokaMapError::Thread("mutex lock failed".to_string()))?
            .pin_mut()
            .init_with_path(&path_str)
            .map_err(|e| TokaMapError::Configuration(format!("Initialization failed: {}", e)))
    }

    /// Initialize with a JSON configuration
    pub fn init_with_json(&mut self, config: &serde_json::Value) -> Result<(), TokaMapError> {
        let config_str = serde_json::to_string(config).map_err(|e| {
            TokaMapError::Configuration(format!("JSON serialization failed: {}", e))
        })?;

        self.inner
            .lock()
            .map_err(|_| TokaMapError::Thread("mutex lock failed".to_string()))?
            .pin_mut()
            .init_with_json(&config_str)
            .map_err(|e| TokaMapError::Configuration(format!("Initialization failed: {}", e)))
    }

    /// Map data with the specified parameters
    pub fn map_data(
        &mut self,
        experiment: &str,
        path: &str,
        data_type: DataType,
        rank: i32,
        extra_attributes: Option<&serde_json::Value>,
    ) -> Result<TypedDataArray, TokaMapError> {
        let data_type_index = data_type as u32;
        let attrs_str = match extra_attributes {
            Some(attrs) => serde_json::to_string(attrs).map_err(|e| {
                TokaMapError::Parameter(format!("Failed to serialize attributes: {}", e))
            })?,
            None => "{}".to_string(),
        };

        let rust_array = self
            .inner
            .lock()
            .map_err(|_| TokaMapError::Thread("mutex lock failed".to_string()))?
            .pin_mut()
            .map_data(experiment, path, data_type_index, rank, &attrs_str)
            .map_err(|e| TokaMapError::Processing(format!("Mapping failed: {}", e)))?;

        Ok(TypedDataArray {
            data_type: match rust_array.data_type {
                1 => DataType::Int8,
                2 => DataType::Int16,
                3 => DataType::Int32,
                4 => DataType::Int64,
                5 => DataType::UInt8,
                6 => DataType::UInt16,
                7 => DataType::UInt32,
                8 => DataType::UInt64,
                9 => DataType::Float,
                10 => DataType::Double,
                _ => DataType::Unknown,
            },
            size: rust_array.size,
            shape: rust_array.shape,
            data: rust_array.data,
        })
    }

    /// Register a data source factory from a dynamic library
    pub fn register_data_source_factory_from_lib(
        &mut self,
        factory_name: &str,
        library_path: impl AsRef<std::path::Path>,
    ) -> Result<(), TokaMapError> {
        let path_str = library_path.as_ref().to_string_lossy();
        self.inner
            .lock()
            .map_err(|_| TokaMapError::Thread("mutex lock failed".to_string()))?
            .pin_mut()
            .register_data_source_factory_with_lib(factory_name, &path_str)
            .map_err(|e| TokaMapError::DataSource(format!("Factory registration failed: {}", e)))
    }

    /// Register a data source with a factory
    pub fn register_data_source_with_factory(
        &mut self,
        name: &str,
        factory_name: &str,
        args: &DataSourceFactoryArgs,
    ) -> Result<(), TokaMapError> {
        let args_json = serde_json::to_string(args).map_err(|e| {
            TokaMapError::Parameter(format!("Failed to serialize factory args: {}", e))
        })?;

        let rust_args = ffi::RustDataSourceFactoryArgs {
            json_string: args_json,
        };

        self.inner
            .lock()
            .map_err(|_| TokaMapError::Thread("mutex lock failed".to_string()))?
            .pin_mut()
            .register_data_source_with_factory(name, factory_name, &rust_args)
            .map_err(|e| {
                TokaMapError::DataSource(format!("Data source registration failed: {}", e))
            })
    }

    /// Unregister a data source
    pub fn unregister_data_source(&mut self, name: &str) -> Result<(), TokaMapError> {
        self.inner
            .lock()
            .map_err(|_| TokaMapError::Thread("mutex lock failed".to_string()))?
            .pin_mut()
            .unregister_data_source(name)
            .map_err(|e| {
                TokaMapError::DataSource(format!("Data source unregistration failed: {}", e))
            })
    }

    /// Load custom function library
    pub fn load_custom_function_library(
        &mut self,
        library_path: impl AsRef<std::path::Path>,
    ) -> Result<(), TokaMapError> {
        let path_str = library_path.as_ref().to_string_lossy();
        self.inner
            .lock()
            .map_err(|_| TokaMapError::Thread("mutex lock failed".to_string()))?
            .pin_mut()
            .load_custom_function_library(&path_str)
            .map_err(|e| {
                TokaMapError::Generic(format!("Custom function library loading failed: {}", e))
            })
    }

    /// Unregister a custom function
    pub fn unregister_custom_function(
        &mut self,
        library_name: &str,
        function_name: &str,
    ) -> Result<(), TokaMapError> {
        self.inner
            .lock()
            .map_err(|_| TokaMapError::Thread("mutex lock failed".to_string()))?
            .pin_mut()
            .unregister_custom_function(library_name, function_name)
            .map_err(|e| {
                TokaMapError::Generic(format!("Custom function unregistration failed: {}", e))
            })
    }
}

impl Default for MappingHandler {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_mapping_handler_creation() {
        let handler = MappingHandler::new();
        // Just test that we can create the handler
        assert!(!handler.inner.lock().unwrap().is_null());
    }

    #[test]
    fn test_data_type_enum() {
        assert_eq!(DataType::Float as u32, 9);
        assert_eq!(DataType::Double as u32, 10);
    }
}
