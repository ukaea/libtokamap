# Creating a New Data Source

This guide explains how to implement custom data sources for LibTokaMap. Data sources are pluggable components that retrieve data from various backends such as databases, files, network APIs, or any custom data format.

## Overview

A data source in LibTokaMap is a class that inherits from `libtokamap::DataSource` and implements the `get()` method to retrieve data based on mapping arguments.

## Basic Data Source Implementation

### Step 1: Create the Header File

Create a header file for your data source (e.g., `my_data_source.hpp`):

```cpp
#pragma once

#include <libtokamap.hpp>
#include <string>
#include <unordered_map>

class MyDataSource : public libtokamap::DataSource
{
public:
    // Constructor - configure your data source
    explicit MyDataSource(const std::string& connection_string);

    // Destructor
    ~MyDataSource() override = default;

    // Main method - implement data retrieval logic
    libtokamap::TypedDataArray get(
        const libtokamap::DataSourceArgs& map_args,
        const libtokamap::MapArguments& arguments,
        libtokamap::RamCache* ram_cache
    ) override;

private:
    std::string m_connection_string;
    // Add any private members needed for your data source
};
```

### Step 2: Implement the Data Source

Create the implementation file (e.g., `my_data_source.cpp`):

```cpp
#include "my_data_source.hpp"
#include <iostream>
#include <vector>

MyDataSource::MyDataSource(const std::string& connection_string)
    : m_connection_string(connection_string)
{
    // Initialize your data source
    // Validate connection, open files, establish connections, etc.
    if (connection_string.empty()) {
        throw libtokamap::DataSourceError("Connection string cannot be empty");
    }
}

libtokamap::TypedDataArray MyDataSource::get(
    const libtokamap::DataSourceArgs& map_args,
    const libtokamap::MapArguments& arguments,
    libtokamap::RamCache* ram_cache)
{
    try {
        // 1. Parse arguments from map_args
        std::string data_path;
        if (map_args.contains("path")) {
            data_path = map_args.at("path").get<std::string>();
        } else {
            throw libtokamap::DataSourceError("Missing 'path' argument");
        }

        // 2. Use cache if available
        std::string cache_key = generate_cache_key(data_path, arguments);
        if (ram_cache) {
            auto cached_data = ram_cache->get(cache_key);
            if (cached_data.has_value()) {
                return cached_data.value();
            }
        }

        // 3. Retrieve data from your source
        std::vector<double> data = retrieve_data(data_path, arguments);

        // 4. Create TypedDataArray result
        libtokamap::TypedDataArray result = data;

        // 5. Store in cache
        if (ram_cache) {
            ram_cache->put(cache_key, result);
        }

        return result;

    } catch (const std::exception& e) {
        throw libtokamap::DataSourceError("Failed to retrieve data: " + std::string(e.what()));
    }
}

// Private helper methods
std::string MyDataSource::generate_cache_key(
    const std::string& path,
    const libtokamap::MapArguments& arguments) const
{
    // Create a unique cache key based on path and arguments
    std::string key = m_connection_string + ":" + path;
    // Add relevant arguments to key if needed
    return key;
}

std::vector<double> MyDataSource::retrieve_data(
    const std::string& path,
    const libtokamap::MapArguments& arguments) const
{
    // Implement your actual data retrieval logic here
    // This could involve:
    // - Reading from files
    // - Querying databases
    // - Making network requests
    // - Processing binary data
    // etc.

    std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0};
    return data;
}
```

## Advanced Data Source Features

### Handling Different Data Types

Your data source should be able to return different data types based on the content:

```cpp
libtokamap::TypedDataArray MyDataSource::get(
    const libtokamap::DataSourceArgs& map_args,
    const libtokamap::MapArguments& arguments,
    libtokamap::RamCache* ram_cache)
{
    std::string data_type = map_args.value("type", "double");
    std::string path = map_args.at("path").get<std::string>();

    if (data_type == "double") {
        std::vector<double> data = retrieve_double_data(path);
        return libtokamap::TypedDataArray{data};
    } else if (data_type == "int") {
        std::vector<int> data = retrieve_int_data(path);
        return libtokamap::TypedDataArray{data};
    } else if (data_type == "string") {
        std::vector<std::string> data = retrieve_string_data(path);
        return libtokamap::TypedDataArray{data};
    } else {
        throw libtokamap::DataSourceError("Unsupported data type: " + data_type);
    }
}
```

### Using MapArguments

The `MapArguments` parameter contains context information that can be used for data retrieval:

```cpp
libtokamap::TypedDataArray MyDataSource::get(
    const libtokamap::DataSourceArgs& map_args,
    const libtokamap::MapArguments& arguments,
    libtokamap::RamCache* ram_cache)
{
    // Access shot number, timestamp, or other contextual information
    if (arguments.extra_attributes.contains("shot")) {
        int shot_number = arguments.extra_attributes["shot"].get<int>();
        // Use shot number to determine data location
    }

    if (arguments.extra_attributes.contains("timestamp")) {
        std::string timestamp = arguments.extra_attributes["timestamp"].get<std::string>();
        // Filter data by timestamp
    }

    // Use the data_path from arguments
    std::string full_path = arguments.data_path;

    // Implement your data retrieval logic...
}
```

### Error Handling

Implement comprehensive error handling:

```cpp
libtokamap::TypedDataArray MyDataSource::get(
    const libtokamap::DataSourceArgs& map_args,
    const libtokamap::MapArguments& arguments,
    libtokamap::RamCache* ram_cache)
{
    try {
        // Validate required arguments
        if (!map_args.contains("path")) {
            throw libtokamap::DataSourceError("Missing required 'path' argument");
        }

        std::string path = map_args.at("path").get<std::string>();

        // Validate path
        if (path.empty()) {
            throw libtokamap::DataSourceError("Path cannot be empty");
        }

        // Attempt data retrieval
        auto data = retrieve_data(path, arguments);

        // Validate data
        if (data.empty()) {
            throw libtokamap::DataSourceError("No data found for path: " + path);
        }

        return libtokamap::TypedDataArray{data};

    } catch (const libtokamap::DataSourceError&) {
        // Re-throw LibTokaMap errors
        throw;
    } catch (const std::exception& e) {
        // Wrap other exceptions
        throw libtokamap::DataSourceError("Data retrieval failed: " + std::string(e.what()));
    }
}
```

## Registration Methods

### Method 1: Direct Registration

Register your data source directly with the mapping handler:

```cpp
#include "my_data_source.hpp"

int main() {
    // Create your data source
    auto data_source = std::make_unique<MyDataSource>("connection_string");

    // Register it with the system
    libtokamap::DataSourceMapping::register_data_source("MY_SOURCE", std::move(data_source));

    // Initialize mapping handler
    libtokamap::MappingHandler handler;
    // ... continue with normal usage
}
```

### Method 2: Factory Function Registration

For more flexibility, create a factory function:

```cpp
// In your data source file
std::unique_ptr<libtokamap::DataSource> create_my_data_source(
    const libtokamap::DataSourceFactoryArgs& args)
{
    auto connection_string = libtokamap::get_arg<std::string>(args, "connection_string");
    return std::make_unique<MyDataSource>(connection_string);
}

// Register the factory
int main() {
    libtokamap::DataSourceMapping::register_data_source_factory(
        "MY_SOURCE",
        create_my_data_source
    );

    // Now you can configure the data source through JSON
}
```

### Method 3: Dynamic Library (Plugin)

For maximum flexibility, create a shared library:

```cpp
// my_data_source_plugin.cpp
#include <libtokamap.hpp>

// Lets LibTokaMap refuse to load the plugin if it was built against an incompatible LibTokaMap
LIBTOKAMAP_EXPORT_FACTORY_ABI_VERSION()

LIBTOKAMAP_FACTORY_EXPORT void LibTokaMapFactoryLoader(libtokamap::FactoryEntryInterface& entry)
{
    entry.function = create_my_data_source;
}
```

Build as a shared library and reference it from the configuration file:

```toml
[data_source_factories]
my_factory = "/path/to/libmy_data_source.so"

[data_sources.MY_SOURCE]
factory = "my_factory"
args.connection_string = "..."
```

Factory arguments that are strings, booleans, integers or floats are passed as `std::string`, `bool`, `int` and
`double`. Tables, arrays and nulls are passed as `nlohmann::json`.

Data sources can also be written in Python and loaded the same way, see
[Python Data Sources](python-data-sources.md).

## Real-World Examples

### JSON File Data Source

```cpp
class JSONDataSource : public libtokamap::DataSource
{
public:
    explicit JSONDataSource(const std::filesystem::path& data_root)
        : m_data_root(data_root)
    {
        if (!std::filesystem::exists(m_data_root)) {
            throw libtokamap::FileError("Data root does not exist");
        }
    }

    libtokamap::TypedDataArray get(
        const libtokamap::DataSourceArgs& map_args,
        const libtokamap::MapArguments& arguments,
        libtokamap::RamCache* ram_cache) override
    {
        std::string filename = map_args.at("file").get<std::string>();
        std::string json_path = map_args.at("path").get<std::string>();

        std::filesystem::path full_path = m_data_root / filename;

        // Load JSON file
        std::ifstream file(full_path);
        if (!file.is_open()) {
            throw libtokamap::FileError("Cannot open file: " + full_path.string());
        }

        nlohmann::json data;
        file >> data;

        // Navigate to specified path
        nlohmann::json::json_pointer ptr(json_path);
        nlohmann::json result = data[ptr];

        // Convert to appropriate data type
        if (result.is_array() && !result.empty()) {
            if (result[0].is_number_float()) {
                return libtokamap::TypedDataArray{result.get<std::vector<double>>()};
            } else if (result[0].is_number_integer()) {
                return libtokamap::TypedDataArray{result.get<std::vector<int>>()};
            }
        }

        throw libtokamap::DataSourceError("Unsupported JSON data type");
    }

private:
    std::filesystem::path m_data_root;
};
```

### Database Data Source

```cpp
class SQLiteDataSource : public libtokamap::DataSource
{
public:
    explicit SQLiteDataSource(const std::string& db_path)
        : m_db_path(db_path)
    {
        // Initialize database connection
        int rc = sqlite3_open(db_path.c_str(), &m_db);
        if (rc) {
            throw libtokamap::DataSourceError("Cannot open database: " + std::string(sqlite3_errmsg(m_db)));
        }
    }

    ~SQLiteDataSource() override {
        if (m_db) {
            sqlite3_close(m_db);
        }
    }

    libtokamap::TypedDataArray get(
        const libtokamap::DataSourceArgs& map_args,
        const libtokamap::MapArguments& arguments,
        libtokamap::RamCache* ram_cache) override
    {
        std::string table = map_args.at("table").get<std::string>();
        std::string column = map_args.at("column").get<std::string>();

        // Build query
        std::string query = "SELECT " + column + " FROM " + table;

        // Add WHERE clause based on arguments
        if (arguments.extra_attributes.contains("shot")) {
            int shot = arguments.extra_attributes["shot"].get<int>();
            query += " WHERE shot = " + std::to_string(shot);
        }

        // Execute query
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(m_db, query.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            throw libtokamap::DataSourceError("SQL error: " + std::string(sqlite3_errmsg(m_db)));
        }

        std::vector<double> results;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            double value = sqlite3_column_double(stmt, 0);
            results.push_back(value);
        }

        sqlite3_finalize(stmt);
        return libtokamap::TypedDataArray{results};
    }

private:
    std::string m_db_path;
    sqlite3* m_db = nullptr;
};
```

## Configuration in Mappings

Once registered, use your data source in mapping configurations:

```json
{
    "my_data": {
        "map_type": "DATA_SOURCE",
        "data_source": "MY_SOURCE",
        "args": {
            "path": "sensors/temperature",
            "format": "csv",
            "delimiter": ","
        },
        "scale": 1.0,
        "offset": 0.0
    }
}
```

## Best Practices

### Performance Optimization

1. **Implement caching**: Always use the provided RAM cache
2. **Minimize I/O**: Batch operations when possible
3. **Connection pooling**: Reuse database connections
4. **Lazy loading**: Load data only when needed

### Error Handling

1. **Validate inputs**: Check all required arguments
2. **Use appropriate exceptions**: Use LibTokaMap exception types
3. **Provide context**: Include helpful error messages
4. **Handle resources**: Ensure proper cleanup in destructors

### Configuration Flexibility

1. **Support multiple formats**: Allow different data formats
2. **Parameterize behavior**: Use arguments for configuration
3. **Document arguments**: Clearly document expected arguments
4. **Provide defaults**: Set reasonable default values

### Testing

Create unit tests for your data source:

```cpp
#include <gtest/gtest.h>
#include "my_data_source.hpp"

class MyDataSourceTest : public ::testing::Test {
protected:
    void SetUp() override {
        data_source = std::make_unique<MyDataSource>("test_connection");
    }

    std::unique_ptr<MyDataSource> data_source;
};

TEST_F(MyDataSourceTest, RetrievesValidData) {
    libtokamap::DataSourceArgs args = {
        {"path", "test/data"}
    };

    libtokamap::MapArguments arguments;
    arguments.data_path = "test/data";

    auto result = data_source->get(args, arguments, nullptr);

    ASSERT_TRUE(result.holds_alternative<std::vector<double>>());
    auto data = std::get<std::vector<double>>(result);
    EXPECT_FALSE(data.empty());
}

TEST_F(MyDataSourceTest, ThrowsOnInvalidPath) {
    libtokamap::DataSourceArgs args = {
        {"path", ""}
    };

    libtokamap::MapArguments arguments;

    EXPECT_THROW(
        data_source->get(args, arguments, nullptr),
        libtokamap::DataSourceError
    );
}
```

## Troubleshooting

### Common Issues

1. **Registration not found**: Ensure data source is registered before use
2. **Missing arguments**: Check that all required arguments are provided
3. **Type mismatches**: Verify returned data type matches expected type
4. **Memory leaks**: Ensure proper resource management in destructor
5. **Thread safety**: Consider thread safety if used in multi-threaded environments

### Debugging Tips

1. **Add logging**: Use appropriate logging framework
2. **Test incrementally**: Test each component separately
3. **Use debugger**: Step through data retrieval logic
4. **Validate data**: Check data integrity at each step
5. **Monitor performance**: Profile data access patterns

This guide provides a comprehensive foundation for creating custom data sources in LibTokaMap. Adapt the examples to your specific data backend requirements.
