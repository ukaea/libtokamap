#pragma once

#include <stdexcept>
#include <string>

namespace libtokamap {

/**
 * Base exception class for all libtokamap errors
 *
 * This serves as the root of the exception hierarchy for the libtokamap library.
 * All specific exception types inherit from this class, which itself inherits
 * from std::runtime_error for compatibility with existing error handling code.
 */
class TokaMapError : public std::runtime_error {
public:
    explicit TokaMapError(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * Configuration and initialization related errors
 *
 * Thrown when there are issues with configuration parameters, missing required
 * settings, or initialization failures.
 *
 * Examples:
 * - Missing mapping_directory in config
 * - Invalid configuration values
 * - Initialization state errors
 */
class ConfigurationError : public TokaMapError {
public:
    explicit ConfigurationError(const std::string& message)
        : TokaMapError("Configuration error: " + message) {}
};

/**
 * Mapping definition and resolution errors
 *
 * Thrown when there are problems with mapping definitions, mapping resolution,
 * or mapping validation.
 *
 * Examples:
 * - No mappings found for a experiment/group combination
 * - Invalid mapping types
 * - Missing required mapping arguments
 * - Failed mapping resolution
 */
class MappingError : public TokaMapError {
public:
    explicit MappingError(const std::string& message)
        : TokaMapError("Mapping error: " + message) {}
};

class MissingMappingError : public MappingError {
public:
    explicit MissingMappingError(const std::string& message)
        : MappingError(message) {}
};

/**
 * Data source related errors
 *
 * Thrown when there are issues with data sources, including registration,
 * availability, or data source-specific operations.
 *
 * Examples:
 * - Unregistered data sources
 * - Data source connection failures
 * - Data source configuration errors
 */
class DataSourceError : public TokaMapError {
public:
    explicit DataSourceError(const std::string& message)
        : TokaMapError("Data source error: " + message) {}
};

class PythonCallbackError : public DataSourceError {
public:
    explicit PythonCallbackError(const std::string& message)
        : DataSourceError("Python callback failed: " + message) {}
};

/**
 * File I/O operation errors
 *
 * Thrown when file operations fail, including reading, writing, or accessing
 * mapping files, configuration files, or data files.
 *
 * Examples:
 * - Cannot open configuration files
 * - Failed to read mapping files
 * - File access permission errors
 */
class FileError : public TokaMapError {
public:
    explicit FileError(const std::string& message)
        : TokaMapError("File error: " + message) {}
};

/**
 * JSON parsing and validation errors
 *
 * Thrown when JSON parsing fails, JSON validation errors occur, or when
 * working with malformed JSON data.
 *
 * Examples:
 * - Invalid JSON syntax
 * - JSON schema validation failures
 * - Unexpected JSON structure
 */
class JsonError : public TokaMapError {
public:
    explicit JsonError(const std::string& message)
        : TokaMapError("JSON error: " + message) {}
};

/**
 * Data type conversion and validation errors
 *
 * Thrown when there are type-related errors, including type mismatches,
 * unsupported data types, or type conversion failures.
 *
 * Examples:
 * - Type index mismatches
 * - Unsupported data type operations
 * - Invalid type conversions
 */
class DataTypeError : public TokaMapError {
public:
    explicit DataTypeError(const std::string& message)
        : TokaMapError("Data type error: " + message) {}
};

/**
 * Path parsing and validation errors
 *
 * Thrown when mapping paths cannot be parsed, are malformed, or contain
 * invalid components.
 *
 * Examples:
 * - Mapping path parsing failures
 * - Invalid path components
 * - Path resolution errors
 */
class PathError : public TokaMapError {
public:
    explicit PathError(const std::string& message)
        : TokaMapError("Path error: " + message) {}
};

/**
 * Schema validation errors
 *
 * Thrown when schema validation fails, including invalid schema formats,
 * missing required fields, or invalid field types.
 *
 * Examples:
 * - Invalid schema formats
 * - Missing required fields
 * - Invalid field types
 */
class SchemaError : public TokaMapError {
public:
    explicit SchemaError(const std::string& message)
        : TokaMapError("Schema error: " + message) {}
};

/**
 * Parameter validation errors
 *
 * Thrown when required parameters are missing, parameters have invalid values,
 * or parameter validation fails.
 *
 * Examples:
 * - Missing required arguments
 * - Invalid parameter values
 * - Parameter range validation failures
 */
class ParameterError : public TokaMapError {
public:
    explicit ParameterError(const std::string& message)
        : TokaMapError("Parameter error: " + message) {}
};

/**
 * Data processing and transformation errors
 *
 * Thrown during data processing operations, including subsetting, scaling,
 * offset operations, and other data transformations.
 *
 * Examples:
 * - Invalid subset operations
 * - Dimension size validation errors
 * - Data transformation failures
 */
class ProcessingError : public TokaMapError {
public:
    explicit ProcessingError(const std::string& message)
        : TokaMapError("Processing error: " + message) {}
};

} // namespace libtokamap
