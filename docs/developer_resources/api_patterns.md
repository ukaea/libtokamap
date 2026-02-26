# LibTokaMap API Patterns and Usage Reference

This document provides comprehensive API patterns and usage examples for LibTokaMap, organized by common use cases and design patterns.

## Core API Patterns

### 1. Basic Library Initialization

```cpp
#include <libtokamap.hpp>

// Method 1: Configuration from TOML file (preferred)
libtokamap::MappingHandler mapping_handler;
mapping_handler.init("/path/to/config.toml");

// Method 2: Configuration from JSON object (legacy)
libtokamap::MappingHandler mapping_handler;
nlohmann::json config = {
    {"mapping_directory", "/path/to/mappings"},
    {"schemas_directory", "/path/to/schemas"},
    {"cache_enabled", true},
    {"cache_size", 100}
};
mapping_handler.init(config);

// Method 3: TOML configuration with factories
// config.toml:
/*
mapping_directory = "/path/to/mappings"
schemas_directory = "/path/to/schemas"
cache_enabled = true
custom_function_libraries = ["/path/to/libcustom.so"]

[data_source_factories]
json_factory = "/path/to/libjson_source.so"
hdf5_factory = "/path/to/libhdf5_source.so"

[data_sources.JSON]
factory = "json_factory"
args.data_root = "/path/to/json/data"

[data_sources.HDF5]
factory = "hdf5_factory"
args.file_path = "/path/to/data.h5"
*/

libtokamap::MappingHandler mapping_handler;
mapping_handler.init("/path/to/config.toml");
```

### 2. Data Source Registration Patterns

```cpp
// Pattern 1: Factory-based registration (preferred)
// Register factory from dynamic library
mapping_handler.register_data_source_factory("json_factory", "/path/to/libjson_source.so");

// Create data source using factory
libtokamap::DataSourceFactoryArgs args = {{"data_root", "/path/to/data"}};
mapping_handler.register_data_source("JSON", "json_factory", args);

// Pattern 2: Direct factory registration
extern "C" std::unique_ptr<libtokamap::DataSource> create_json_source(
    const libtokamap::DataSourceFactoryArgs& args) {
    std::string data_root = args.at("data_root").get<std::string>();
    return std::make_unique<JSONDataSource>(data_root);
}

auto factory = libtokamap::DataSourceFactory{create_json_source};
mapping_handler.register_data_source_factory("json_factory", factory);

// Pattern 3: Legacy direct registration (still supported)
auto json_source = std::make_unique<JSONDataSource>("/path/to/data");
mapping_handler.register_data_source("JSON", std::move(json_source));

// Pattern 4: Custom data source implementation with factory
class CustomDataSource : public libtokamap::DataSource {
public:
    explicit CustomDataSource(const libtokamap::DataSourceFactoryArgs& args) {
        // Initialize from factory args
        config_param = args.at("config_param").get<std::string>();
    }
    
    libtokamap::TypedDataArray get(
        const libtokamap::DataSourceArgs& args,
        const libtokamap::MapArguments& arguments,
        libtokamap::RamCache* ram_cache) override {
        
        // Implementation specific logic
        std::vector<double> data = fetch_data(args, config_param);
        return libtokamap::TypedDataArray{data};
    }
    
private:
    std::string config_param;
};

// Factory function for custom data source
extern "C" std::unique_ptr<libtokamap::DataSource> create_custom_source(
    const libtokamap::DataSourceFactoryArgs& args) {
    return std::make_unique<CustomDataSource>(args);
}

// Pattern 5: TOML-driven multiple source registration
void register_sources_from_toml(libtokamap::MappingHandler& handler) {
    // This happens automatically when using init() with TOML config
    handler.init("/path/to/config.toml");
    
    // All data_source_factories and data_sources from TOML are registered
    // No manual registration needed
}
```

### 3. Mapping Request Patterns

```cpp
// Pattern 1: Basic mapping request
std::type_index data_type = std::type_index{typeid(double)};
int rank = 1;
nlohmann::json extra_attributes = {{"shot", 12345}, {"time", "2023-01-01"}};

auto result = mapping_handler.map(
    "EXPERIMENT_NAME",
    "path/to/data",
    data_type,
    rank,
    extra_attributes
);

// Pattern 2: Type-safe result extraction
if (result.type_index() == std::type_index{typeid(double)}) {
    auto data_vector = result.to_vector<double>();
    auto data_span = result.span<double>();
    const double* raw_data = result.data<double>();
}

// Pattern 3: Multi-dimensional data handling
std::type_index data_type = std::type_index{typeid(float)};
int rank = 2; // 2D array expected
auto result = mapping_handler.map("EXPERIMENT", "2d/array/path", data_type, rank, attributes);

// Access shape information
const auto& shape = result.shape();
size_t rows = shape[0];
size_t cols = shape[1];
```

## Error Handling Patterns

### 1. Exception Handling Strategy

```cpp
#include <exceptions/exceptions.hpp>

// Pattern 1: Comprehensive error handling
try {
    auto result = mapping_handler.map("EXPERIMENT", "path", type, rank, attrs);
    process_result(result);
} catch (const libtokamap::MappingError& e) {
    std::cerr << "Mapping error: " << e.what() << std::endl;
    // Handle mapping-specific errors
} catch (const libtokamap::DataSourceError& e) {
    std::cerr << "Data source error: " << e.what() << std::endl;
    // Handle data source errors
} catch (const libtokamap::ValidationError& e) {
    std::cerr << "Validation error: " << e.what() << std::endl;
    // Handle configuration validation errors
} catch (const libtokamap::TokaMapError& e) {
    std::cerr << "General error: " << e.what() << std::endl;
    // Handle any other library errors
}

// Pattern 2: Error recovery with fallbacks
libtokamap::TypedDataArray safe_map(
    libtokamap::MappingHandler& handler,
    const std::string& experiment,
    const std::string& path,
    std::type_index type,
    int rank,
    const nlohmann::json& attrs) {
    
    try {
        return handler.map(experiment, path, type, rank, attrs);
    } catch (const libtokamap::MappingError&) {
        // Try alternative mapping
        std::string fallback_path = path + "_fallback";
        try {
            return handler.map(experiment, fallback_path, type, rank, attrs);
        } catch (const libtokamap::MappingError&) {
            // Return default value
            if (type == std::type_index{typeid(double)}) {
                return libtokamap::TypedDataArray{0.0};
            }
            throw; // Re-throw if no fallback possible
        }
    }
}
```

### 2. Validation and Debugging Patterns

```cpp
// Pattern 1: Pre-validation
bool validate_request(const std::string& experiment, const std::string& path) {
    // Check experiment exists
    // Validate path format
    // Check permissions
    return true;
}

// Pattern 2: Debug information extraction
void debug_mapping_result(const libtokamap::TypedDataArray& result) {
    std::cout << "Result type: " << result.type_index().name() << std::endl;
    std::cout << "Size: " << result.size() << std::endl;
    std::cout << "Rank: " << result.rank() << std::endl;
    std::cout << "Shape: [";
    for (size_t i = 0; i < result.shape().size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << result.shape()[i];
    }
    std::cout << "]" << std::endl;
    std::cout << "Data preview: " << result.to_string(5) << std::endl;
}
```

## Advanced Usage Patterns

### 1. Custom Mapping Functions

```cpp
// Pattern 1: External library function definition
extern "C" libtokamap::TypedDataArray custom_interpolation(
    const libtokamap::CustomMappingInputMap& inputs,
    const libtokamap::CustomMappingParams& params) {
    
    // Extract input data
    auto x_data = inputs.at("x_values");
    auto y_data = inputs.at("y_values");
    
    // Extract parameters
    auto method = params.at("method").get<std::string>();
    auto num_points = params.at("num_points").get<int>();
    
    // Perform interpolation
    std::vector<double> result = perform_interpolation(x_data, y_data, method, num_points);
    
    return libtokamap::TypedDataArray{result};
}

// Pattern 2: Dynamic library registration
void register_custom_functions(libtokamap::MappingHandler& handler) {
    std::vector<std::string> library_paths = {
        "/usr/local/lib/libtokamap_extensions.so",
        "./custom_functions.so"
    };
    
    for (const auto& lib_path : library_paths) {
        try {
            auto func = load_library_function(lib_path, "custom_interpolation");
            handler.register_custom_function(func);
        } catch (const std::exception& e) {
            std::cerr << "Failed to load " << lib_path << ": " << e.what() << std::endl;
        }
    }
}
```

### 2. Performance Optimization Patterns

```cpp
// Pattern 1: Cache management
class OptimizedMappingHandler {
private:
    libtokamap::MappingHandler handler;
    std::unordered_map<std::string, libtokamap::TypedDataArray> local_cache;
    
public:
    libtokamap::TypedDataArray cached_map(
        const std::string& experiment,
        const std::string& path,
        std::type_index type,
        int rank,
        const nlohmann::json& attrs) {
        
        // Create cache key
        std::string cache_key = experiment + "|" + path + "|" + 
                               std::to_string(type.hash_code());
        
        // Check local cache first
        auto it = local_cache.find(cache_key);
        if (it != local_cache.end()) {
            return std::move(it->second);
        }
        
        // Fetch and cache
        auto result = handler.map(experiment, path, type, rank, attrs);
        local_cache[cache_key] = std::move(result);
        return local_cache[cache_key];
    }
    
    void clear_cache() {
        local_cache.clear();
        // Also clear internal cache if needed
    }
};

// Pattern 2: Batch processing
class BatchProcessor {
public:
    struct BatchRequest {
        std::string experiment;
        std::string path;
        std::type_index type;
        int rank;
        nlohmann::json attributes;
    };
    
    std::vector<libtokamap::TypedDataArray> process_batch(
        libtokamap::MappingHandler& handler,
        const std::vector<BatchRequest>& requests) {
        
        std::vector<libtokamap::TypedDataArray> results;
        results.reserve(requests.size());
        
        for (const auto& request : requests) {
            try {
                results.emplace_back(handler.map(
                    request.experiment,
                    request.path,
                    request.type,
                    request.rank,
                    request.attributes
                ));
            } catch (const std::exception& e) {
                std::cerr << "Batch item failed: " << e.what() << std::endl;
                // Add empty result or skip
                results.emplace_back();
            }
        }
        
        return results;
    }
};
```

### Configuration Management Patterns

```cpp
// Pattern 1: TOML-based environment configuration (preferred)
std::string create_environment_toml_config() {
    std::string config_content = R"(
mapping_directory = "/path/to/mappings"
schemas_directory = "/path/to/schemas"
cache_enabled = true
trace_enabled = false
)";

    // Environment-specific overrides
    const char* env = std::getenv("TOKAMAP_ENV");
    if (env && std::string{env} == "production") {
        config_content += R"(
cache_size = 10000
custom_function_libraries = ["/opt/tokamap/lib/libproduction.so"]

[data_source_factories]
json_factory = "/opt/tokamap/lib/libjson_source.so"
hdf5_factory = "/opt/tokamap/lib/libhdf5_source.so"

[data_sources.JSON]
factory = "json_factory"
args.data_root = "/opt/data/production"
)";
    } else if (env && std::string{env} == "development") {
        config_content += R"(
cache_size = 100
trace_enabled = true

[data_source_factories] 
json_factory = "./build/libjson_source.so"

[data_sources.JSON]
factory = "json_factory"
args.data_root = "./test_data"
)";
    }
    
    return config_content;
}

// Pattern 2: Legacy JSON configuration (maintained for compatibility)
nlohmann::json create_environment_config() {
    nlohmann::json config;
    
    // Base configuration
    config["mapping_directory"] = "/path/to/mappings";
    config["schemas_directory"] = "/path/to/schemas";
    config["cache_enabled"] = true;
    config["cache_size"] = 1000;
    
    // Environment-specific overrides
    const char* env = std::getenv("TOKAMAP_ENV");
    if (env && std::string{env} == "production") {
        config["mapping_directory"] = "/opt/tokamap/mappings";
        config["cache_size"] = 10000;
    } else if (env && std::string{env} == "development") {
        config["mapping_directory"] = "./dev_mappings";
        config["cache_size"] = 100;
        config["trace_enabled"] = true;
    }
    
    return config;
}

// Pattern 3: Configuration validation with enhanced schema
bool validate_configuration(const nlohmann::json& config) {
    // Required fields
    std::vector<std::string> required_fields = {"mapping_directory", "schemas_directory"};
    for (const auto& field : required_fields) {
        if (!config.contains(field)) {
            std::cerr << "Missing required field: " << field << std::endl;
            return false;
        }
    }
    
    // Path validation
    std::filesystem::path mapping_dir = config["mapping_directory"];
    if (!std::filesystem::exists(mapping_dir)) {
        std::cerr << "Mapping directory does not exist: " << mapping_dir << std::endl;
        return false;
    }
    
    std::filesystem::path schemas_dir = config["schemas_directory"];
    if (!std::filesystem::exists(schemas_dir)) {
        std::cerr << "Schemas directory does not exist: " << schemas_dir << std::endl;
        return false;
    }
    
    // Factory validation
    if (config.contains("data_source_factories")) {
        for (const auto& [name, path] : config["data_source_factories"].items()) {
            if (!std::filesystem::exists(path.get<std::string>())) {
                std::cerr << "Factory library not found: " << path.get<std::string>() << std::endl;
                return false;
            }
        }
    }
    
    return true;
}

// Pattern 4: TOML configuration builder
class TOMLConfigBuilder {
public:
    TOMLConfigBuilder& mapping_directory(const std::string& dir) {
        config_["mapping_directory"] = dir;
        return *this;
    }
    
    TOMLConfigBuilder& schemas_directory(const std::string& dir) {
        config_["schemas_directory"] = dir;
        return *this;
    }
    
    TOMLConfigBuilder& add_data_source_factory(const std::string& name, const std::string& path) {
        config_["data_source_factories"][name] = path;
        return *this;
    }
    
    TOMLConfigBuilder& add_data_source(const std::string& name, const std::string& factory, 
                                      const nlohmann::json& args) {
        config_["data_sources"][name]["factory"] = factory;
        config_["data_sources"][name]["args"] = args;
        return *this;
    }
    
    std::string build_toml() const {
        // Convert internal JSON to TOML format
        std::ostringstream toml;
        
        toml << "mapping_directory = \"" << config_["mapping_directory"].get<std::string>() << "\"\n";
        toml << "schemas_directory = \"" << config_["schemas_directory"].get<std::string>() << "\"\n\n";
        
        if (config_.contains("data_source_factories")) {
            toml << "[data_source_factories]\n";
            for (const auto& [name, path] : config_["data_source_factories"].items()) {
                toml << name << " = \"" << path.get<std::string>() << "\"\n";
            }
            toml << "\n";
        }
        
        if (config_.contains("data_sources")) {
            for (const auto& [name, ds_config] : config_["data_sources"].items()) {
                toml << "[data_sources." << name << "]\n";
                toml << "factory = \"" << ds_config["factory"].get<std::string>() << "\"\n";
                if (ds_config.contains("args")) {
                    for (const auto& [arg_name, arg_value] : ds_config["args"].items()) {
                        toml << "args." << arg_name << " = \"" << arg_value.get<std::string>() << "\"\n";
                    }
                }
                toml << "\n";
            }
        }
        
        return toml.str();
    }
    
private:
    nlohmann::json config_;
};

// Usage
auto toml_config = TOMLConfigBuilder{}
    .mapping_directory("/path/to/mappings")
    .schemas_directory("/path/to/schemas")
    .add_data_source_factory("json_factory", "/path/to/libjson.so")
    .add_data_source("JSON", "json_factory", {{"data_root", "/path/to/data"}})
    .build_toml();
```

## Integration Patterns

### 1. RAII and Resource Management

```cpp
// Pattern 1: RAII wrapper for MappingHandler
class ScopedMappingHandler {
private:
    libtokamap::MappingHandler handler;
    bool initialized = false;
    
public:
    explicit ScopedMappingHandler(const nlohmann::json& config) {
        try {
            handler.init(config);
            initialized = true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize: " << e.what() << std::endl;
            throw;
        }
    }
    
    ~ScopedMappingHandler() {
        if (initialized) {
            handler.reset();
        }
    }
    
    // Non-copyable, movable
    ScopedMappingHandler(const ScopedMappingHandler&) = delete;
    ScopedMappingHandler& operator=(const ScopedMappingHandler&) = delete;
    ScopedMappingHandler(ScopedMappingHandler&&) = default;
    ScopedMappingHandler& operator=(ScopedMappingHandler&&) = default;
    
    libtokamap::TypedDataArray map(
        const std::string& experiment,
        const std::string& path,
        std::type_index type,
        int rank,
        const nlohmann::json& attrs) {
        if (!initialized) {
            throw std::runtime_error("Handler not initialized");
        }
        return handler.map(experiment, path, type, rank, attrs);
    }
    
    void register_data_source(const std::string& name, 
                             std::unique_ptr<libtokamap::DataSource> source) {
        if (!initialized) {
            throw std::runtime_error("Handler not initialized");
        }
        handler.register_data_source(name, std::move(source));
    }
};

// Usage
{
    ScopedMappingHandler handler{config};
    auto result = handler.map("EXPERIMENT", "path", type, rank, attrs);
    // Automatic cleanup on scope exit
}
```

### 2. Factory Patterns

```cpp
// Pattern 1: Data source factory
class DataSourceFactory {
public:
    static std::unique_ptr<libtokamap::DataSource> create(
        const std::string& type,
        const nlohmann::json& config) {
        
        if (type == "JSON") {
            std::string root_path = config["root_path"];
            return std::make_unique<JSONDataSource>(root_path);
        } else if (type == "HDF5") {
            std::string file_path = config["file_path"];
            return std::make_unique<HDF5DataSource>(file_path);
        } else if (type == "DATABASE") {
            std::string connection = config["connection_string"];
            return std::make_unique<DatabaseSource>(connection);
        }
        
        throw std::invalid_argument("Unknown data source type: " + type);
    }
    
    static void register_all(libtokamap::MappingHandler& handler,
                           const nlohmann::json& sources_config) {
        for (const auto& [name, config] : sources_config.items()) {
            std::string type = config["type"];
            auto source = create(type, config);
            handler.register_data_source(name, std::move(source));
        }
    }
};

// Pattern 2: Configuration factory
class ConfigurationFactory {
public:
    static nlohmann::json from_file(const std::string& path) {
        std::ifstream file{path};
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open config file: " + path);
        }
        
        nlohmann::json config;
        file >> config;
        return config;
    }
    
    static nlohmann::json from_environment() {
        nlohmann::json config;
        
        if (const char* mapping_dir = std::getenv("TOKAMAP_MAPPING_DIR")) {
            config["mapping_directory"] = mapping_dir;
        }
        
        if (const char* cache_size = std::getenv("TOKAMAP_CACHE_SIZE")) {
            config["cache_size"] = std::stoi(cache_size);
        }
        
        if (const char* cache_enabled = std::getenv("TOKAMAP_CACHE_ENABLED")) {
            config["cache_enabled"] = std::string{cache_enabled} == "true";
        }
        
        return config;
    }
    
    static nlohmann::json merge_configs(const std::vector<nlohmann::json>& configs) {
        nlohmann::json merged;
        for (const auto& config : configs) {
            merged.merge_patch(config);
        }
        return merged;
    }
};
```

### 3. Observer/Callback Patterns

```cpp
// Pattern 1: Progress monitoring
class MappingProgressMonitor {
public:
    using ProgressCallback = std::function<void(const std::string&, double)>;
    using ErrorCallback = std::function<void(const std::string&, const std::exception&)>;
    
    void set_progress_callback(ProgressCallback callback) {
        progress_callback = std::move(callback);
    }
    
    void set_error_callback(ErrorCallback callback) {
        error_callback = std::move(callback);
    }
    
    libtokamap::TypedDataArray monitored_map(
        libtokamap::MappingHandler& handler,
        const std::string& experiment,
        const std::string& path,
        std::type_index type,
        int rank,
        const nlohmann::json& attrs) {
        
        std::string operation = experiment + "/" + path;
        
        if (progress_callback) {
            progress_callback(operation, 0.0);
        }
        
        try {
            auto result = handler.map(experiment, path, type, rank, attrs);
            
            if (progress_callback) {
                progress_callback(operation, 1.0);
            }
            
            return result;
        } catch (const std::exception& e) {
            if (error_callback) {
                error_callback(operation, e);
            }
            throw;
        }
    }
    
private:
    ProgressCallback progress_callback;
    ErrorCallback error_callback;
};

// Usage
MappingProgressMonitor monitor;
monitor.set_progress_callback([](const std::string& op, double progress) {
    std::cout << op << ": " << (progress * 100) << "%" << std::endl;
});

monitor.set_error_callback([](const std::string& op, const std::exception& e) {
    std::cerr << "Error in " << op << ": " << e.what() << std::endl;
});

auto result = monitor.monitored_map(handler, "EXP", "path", type, rank, attrs);
```

## Testing Patterns

### 1. Mock Data Sources

```cpp
// Pattern 1: Mock data source for testing
class MockDataSource : public libtokamap::DataSource {
private:
    std::unordered_map<std::string, libtokamap::TypedDataArray> mock_data;
    
public:
    void add_mock_data(const std::string& key, libtokamap::TypedDataArray data) {
        mock_data[key] = std::move(data);
    }
    
    libtokamap::TypedDataArray get(
        const libtokamap::DataSourceArgs& args,
        const libtokamap::MapArguments& arguments,
        libtokamap::RamCache* ram_cache) override {
        
        std::string key = args.at("path").get<std::string>();
        auto it = mock_data.find(key);
        if (it != mock_data.end()) {
            // Return copy of mock data
            return std::move(it->second);
        }
        
        throw libtokamap::DataSourceError("Mock data not found: " + key);
    }
};

// Test usage
void test_mapping_functionality() {
    libtokamap::MappingHandler handler;
    
    // Setup mock data source
    auto mock_source = std::make_unique<MockDataSource>();
    std::vector<double> test_data = {1.0, 2.0, 3.0, 4.0, 5.0};
    mock_source->add_mock_data("test/path", libtokamap::TypedDataArray{test_data});
    
    handler.register_data_source("MOCK", std::move(mock_source));
    
    // Configure handler with test mappings
    nlohmann::json config = {{"mapping_directory", "./test_mappings"}};
    handler.init(config);
    
    // Test mapping
    auto result = handler.map("TEST_EXPERIMENT", "test/path", 
                             std::type_index{typeid(double)}, 1, {});
    
    assert(result.size() == 5);
    assert(result.to_vector<double>() == test_data);
}
```

## Advanced Subset Operation Patterns

### 1. Multi-dimensional Slicing

```cpp
// Pattern 1: Column selection from 2D array
libtokamap::TypedDataArray array_2d{data, {rows, cols}};

// Select column 9: [:][9]
auto result_1d = handler.map("EXPERIMENT", "data/array[:][9]", 
                            std::type_index{typeid(float)}, 1, {});

// Pattern 2: Row range with stride: [2:8:2][:]
auto result_strided = handler.map("EXPERIMENT", "data/array[2:8:2][:]",
                                 std::type_index{typeid(float)}, 2, {});

// Pattern 3: Negative stride (reverse): [10:0:-1]
auto result_reversed = handler.map("EXPERIMENT", "data/array[10:0:-1]",
                                  std::type_index{typeid(float)}, 1, {});

// Pattern 4: Complex 3D slicing: [::2][5:10][1:8:2]
auto result_3d = handler.map("EXPERIMENT", "data/volume[::2][5:10][1:8:2]",
                            std::type_index{typeid(double)}, 3, {});
```

### 2. TypedDataArray Enhanced Operations

```cpp
// Pattern 1: Array cloning for safe operations
libtokamap::TypedDataArray original{data};
auto cloned = original.clone(); // New method from develop branch

// Modify clone without affecting original
cloned.apply<double>(2.0, 1.0);
assert(original.to_vector<double>() != cloned.to_vector<double>());

// Pattern 2: Safe multi-dimensional slicing with validation
try {
    std::vector<libtokamap::SubsetInfo> subsets = libtokamap::parse_slices("[:][9]", array.shape());
    
    // Validate before applying
    for (size_t i = 0; i < subsets.size(); ++i) {
        if (!subsets[i].validate()) {
            throw std::invalid_argument("Invalid subset for dimension " + std::to_string(i));
        }
    }
    
    array.slice<float>(subsets);
} catch (const libtokamap::ProcessingError& e) {
    std::cerr << "Slicing failed: " << e.what() << std::endl;
}

// Pattern 3: Dimension reduction detection
auto original_rank = array.rank();
array.slice<float>(subsets);
auto new_rank = array.rank();

if (new_rank < original_rank) {
    std::cout << "Dimension reduced from " << original_rank 
              << "D to " << new_rank << "D" << std::endl;
}
```

This comprehensive API patterns reference provides developers with practical examples and best practices for using LibTokaMap effectively in various scenarios, including the new factory-based configuration system, TOML support, and enhanced subset operations introduced in the develop branch.