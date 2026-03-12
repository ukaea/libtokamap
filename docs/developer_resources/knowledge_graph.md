# LibTokaMap Knowledge Graph

## Overview

This knowledge graph captures the architectural components, relationships, and data flow patterns within the LibTokaMap library. It serves as a comprehensive reference for understanding the system architecture and planning refactoring efforts.

## Core Architecture

### 1. Main Components

```mermaid
graph TB
    MH[MappingHandler] --> |manages| ER[ExperimentRegister]
    MH --> |uses| DSR[DataSourceRegistry]
    MH --> |utilizes| RC[RamCache]
    MH --> |validates with| VS[ValidationSchemas]
    
    ER --> |contains| EM[ExperimentMappings]
    EM --> |organized by| GM[GroupMappings]
    GM --> |partitioned by| PM[PartitionMappings]
    PM --> |stores| MS[MappingStore]
    MS --> |contains| M[Mapping implementations]
```

### 2. Mapping Type Hierarchy

```mermaid
classDiagram
    class Mapping {
        <<abstract>>
        +map(MapArguments) TypedDataArray
    }
    
    class ValueMapping {
        -nlohmann::json m_value
        +map(MapArguments) TypedDataArray
    }
    
    class DataSourceMapping {
        -DataSource* m_data_source
        -DataSourceArgs m_args
        -optional~float~ m_offset
        -optional~float~ m_scale
        -optional~string~ m_slice
        +map(MapArguments) TypedDataArray
    }
    
    class ExprMapping {
        -string m_expr
        -unordered_map~string,string~ m_parameters
        +map(MapArguments) TypedDataArray
    }
    
    class DimMapping {
        -string m_dim_probe
        +map(MapArguments) TypedDataArray
    }
    
    class CustomMapping {
        -vector~LibraryFunction~ m_functions
        -LibraryName m_library_name
        -FunctionName m_function_name
        -CustomMappingInputMap m_input_map
        -CustomMappingParams m_params
        +map(MapArguments) TypedDataArray
    }
    
    Mapping <|-- ValueMapping
    Mapping <|-- DataSourceMapping
    Mapping <|-- ExprMapping
    Mapping <|-- DimMapping
    Mapping <|-- CustomMapping
```

### 3. Data Source Architecture

```mermaid
graph LR
    DSR[DataSourceRegistry] --> |contains| DS[DataSource implementations]
    DSF[DataSourceFactory] --> |creates| DS
    MH[MappingHandler] --> |manages| DSF
    DS --> |provides| TDA[TypedDataArray]
    DSM[DataSourceMapping] --> |uses| DS
    DSM --> |applies| SO[ScaleOffset]
    DSM --> |applies| SL[Slicing]
    DSM --> |uses| RC[RamCache]
    
    subgraph "Factory Pattern"
        JSON_FACTORY[JSON Factory]
        HDF5_FACTORY[HDF5 Factory]
        CUSTOM_FACTORY[Custom Factories]
    end
    
    subgraph "Data Source Implementations"
        JSON[JSONDataSource]
        HDF5[HDF5DataSource]
        CUSTOM[CustomDataSources]
    end
    
    DSF --> JSON_FACTORY
    DSF --> HDF5_FACTORY
    DSF --> CUSTOM_FACTORY
    
    JSON_FACTORY --> JSON
    HDF5_FACTORY --> HDF5
    CUSTOM_FACTORY --> CUSTOM
```

## Data Structures and Types

### 1. Core Data Types

| Type | Purpose | Key Features |
|------|---------|--------------|
| `TypedDataArray` | Type-safe data container | Move-only, templated access, shape awareness |
| `MapArguments` | Context for mapping operations | Contains global data, entries, data type, rank |
| `DataType` | Enumeration of supported types | Maps to C++ fundamental types |
| `SubsetInfo` | Slice specification | Start, stop, stride with validation |

### 2. Configuration Types

| Type | Purpose | Structure |
|------|---------|-----------|
| `ExperimentMappings` | Experiment configuration | Partition list, groups, mappings, globals |
| `MappingPartition` | Directory selection logic | Attribute name, selector strategy |
| `DirectorySelector` | Partition selection strategy | MaxBelow, MinAbove, Exact, Closest |
| `DataSourceFactory` | Factory function type | Creates DataSource from args |
| `DataSourceFactoryArgs` | Factory configuration | Parameters for data source creation |

#### Enhanced Configuration Schema

The configuration now supports:
- **Data Source Factories**: Dynamic loading of data source implementations
- **Factory Registration**: TOML-based factory configuration
- **Modular Data Sources**: Plugin-based architecture with hot-loading
- **Custom Function Libraries**: External library loading for custom mappings

### 3. JSON Schema Integration

```yaml
Schemas:
  - mappings.schema.json: Validates mapping definitions
  - globals.schema.json: Validates global variable files
  - mappings.cfg.schema.json: Validates experiment configuration (DEPRECATED)
  - config.schema.json: Validates TOML configuration files

Configuration Formats:
  - TOML: Primary configuration format (preferred)
  - JSON: Legacy support maintained

Validation Flow:
  TOML/JSON Input → Schema Validation → Object Creation → Runtime Usage
```

## Data Flow and Processing Pipeline

### 1. Initialization Flow

```mermaid
sequenceDiagram
    participant Client
    participant MH as MappingHandler
    participant EM as ExperimentMappings
    participant VS as ValidationSchemas
    
    Client->>MH: init(config)
    MH->>VS: load schemas
    MH->>MH: setup data sources
    MH->>MH: setup cache
    MH->>EM: load experiment configs
    EM->>VS: validate configs
    MH-->>Client: ready
```

### 2. Mapping Resolution Flow

```mermaid
sequenceDiagram
    participant Client
    participant MH as MappingHandler
    participant EM as ExperimentMappings
    participant PM as PartitionMappings
    participant M as Mapping
    participant DS as DataSource
    
    Client->>MH: map(experiment, path, type, rank, attrs)
    MH->>EM: get experiment
    EM->>PM: resolve partition
    PM->>M: get mapping
    M->>DS: get data (if DataSourceMapping)
    DS-->>M: TypedDataArray
    M->>M: apply transformations
    M-->>MH: final TypedDataArray
    MH-->>Client: result
```

### 3. Data Transformation Pipeline

```mermaid
graph LR
    DS[Data Source] --> RAW[Raw Data]
    RAW --> SLICE[Apply Slicing]
    SLICE --> SCALE[Apply Scale/Offset]
    SCALE --> CACHE[Cache Result]
    CACHE --> TDA[TypedDataArray]
    
    subgraph "Transformation Options"
        SUBSET[Subset Operations]
        LINEAR[Linear Transforms]
        TEMPLATE[Template Rendering]
    end
    
    SLICE -.-> SUBSET
    SCALE -.-> LINEAR
    RAW -.-> TEMPLATE
```

## Directory Structure and Organization

### 1. Project Layout

```
libtokamap/
├── include/           # Public headers
├── src/              # Implementation
│   ├── handlers/     # MappingHandler
│   ├── map_types/    # Mapping implementations
│   ├── utils/        # Utilities and helpers
│   └── exceptions/   # Exception types
├── examples/         # Usage examples
├── test/            # Unit tests
├── schemas/         # JSON schemas
└── docs/           # Documentation
```

### 2. Mapping Directory Structure

```
mappings/
├── experiment1/
│   ├── mappings.cfg.json      # Experiment configuration
│   ├── globals.json           # Top-level globals
│   └── group_name/
│       └── partition_value/
│           ├── globals.json   # Partition globals
│           └── mappings.json  # Actual mappings
```

## Key Algorithms and Utilities

### 1. Subset Operations

| Operation | Description | Example | Notes |
|-----------|-------------|---------|-------|
| Basic slice | `[start:stop:stride]` | `[0:10:2]` | Standard Python-style slicing |
| Negative indexing | From end of array | `[-5:-1]` | Supports negative indices |
| Negative stride | Reverse iteration | `[10:0:-1]` | Backwards slicing with validation |
| Multi-dimensional | Per-dimension slicing | `[:][9]` | Column/row selection |
| Dimension reduction | Remove singleton dims | `[5:6]` becomes `[5]` | Automatic rank reduction |

#### Enhanced Subset Validation
- Comprehensive validation for negative strides
- Proper handling of edge cases (wraparound prevention)
- Multi-dimensional slicing with rank reduction
- Extensive test coverage for 1D, 2D, and 3D operations

### 2. Template Rendering (Inja)

- Global variable substitution
- Expression evaluation in mapping definitions
- Dynamic path construction

### 3. Caching Strategy

```mermaid
graph TD
    REQUEST[Data Request] --> CACHE{In Cache?}
    CACHE -->|Yes| RETURN[Return Cached]
    CACHE -->|No| FETCH[Fetch from Source]
    FETCH --> STORE[Store in Cache]
    STORE --> RETURN
    
    CACHE --> LRU[LRU Eviction]
    LRU --> CAPACITY[Check Capacity]
```

## Extension Points and Plugin Architecture

### 1. Custom Data Sources

```cpp
class CustomDataSource : public DataSource {
public:
    TypedDataArray get(const DataSourceArgs& args,
                      const MapArguments& arguments,
                      RamCache* cache) override;
};

// Method 1: Direct registration
mapping_handler.register_data_source("MY_SOURCE", 
    std::make_unique<CustomDataSource>());

// Method 2: Factory-based registration
extern "C" std::unique_ptr<DataSource> create_data_source(const DataSourceFactoryArgs& args) {
    return std::make_unique<CustomDataSource>(args);
}

// Register factory and create data source
mapping_handler.register_data_source_factory("MY_FACTORY", "/path/to/libcustom.so");
mapping_handler.register_data_source("MY_SOURCE", "MY_FACTORY", factory_args);
```

### 2. Custom Mapping Functions

```cpp
// External library function signature
extern "C" TypedDataArray my_custom_function(
    const CustomMappingInputMap& inputs,
    const CustomMappingParams& params);

// Dynamic loading and registration
LibraryFunction func = load_library_function("path/to/lib.so", 
                                           "my_custom_function");
mapping_handler.register_custom_function(func);
```

## Error Handling and Exception Hierarchy

```mermaid
classDiagram
    class TokaMapError {
        +string message
    }
    
    class MappingError {
        +MappingError(string)
    }
    
    class DataSourceError {
        +DataSourceError(string)
    }
    
    class ProcessingError {
        +ProcessingError(string)
    }
    
    class ValidationError {
        +ValidationError(string)
    }
    
    class DataTypeError {
        +DataTypeError(string)
    }
    
    class ParameterError {
        +ParameterError(string)
    }
    
    TokaMapError <|-- MappingError
    TokaMapError <|-- DataSourceError
    TokaMapError <|-- ProcessingError
    TokaMapError <|-- ValidationError
    TokaMapError <|-- DataTypeError
    TokaMapError <|-- ParameterError
```

## Dependencies and External Libraries

### 1. Core Dependencies

| Library | Purpose | Usage |
|---------|---------|--------|
| nlohmann/json | JSON parsing | Configuration, data exchange |
| Pantor/Inja | Template engine | Dynamic content generation |
| ExprTk | Expression parsing | Mathematical expressions |
| valijson | JSON validation | Schema validation |

### 2. Build System Integration

- CMake 3.15+ with C++20 support
- Optional components: testing, examples
- Static analysis integration (clang-format, clang-tidy)

## Performance Considerations

### 1. Memory Management

- Move semantics for `TypedDataArray`
- RAII for resource management
- Memory-mapped file access for large datasets
- Copy-on-write for cached data

### 2. Optimization Strategies

- Lazy loading of experiment configurations
- Hierarchical caching (memory → disk → network)
- Template compilation and caching
- Data type specialization for common operations

## Refactoring Opportunities

### 1. Type Safety Improvements

- Replace `std::type_index` with `DataType` enum
- Add C++20 concepts for template constraints
- Strengthen compile-time type checking

### 2. Modern C++ Features

- `std::format` instead of string concatenation
- `std::expected` for error handling
- Coroutines for async data loading
- Modules for better compilation

### 3. Architecture Enhancements

- Immutable configuration objects
- Functional mapping composition
- Reactive data streams
- Plugin hot-reloading

## Testing Strategy

### 1. Test Categories

| Category | Coverage | Examples |
|----------|----------|----------|
| Unit Tests | Individual components | `TypedDataArray`, `SubsetInfo` |
| Integration Tests | Component interaction | Mapping resolution flow |
| Schema Tests | Configuration validation | TOML/JSON schema compliance |
| Performance Tests | Benchmarking | Large dataset processing |
| Subset Tests | Advanced slicing operations | Multi-dimensional, negative strides |

### Enhanced Testing Features

#### Comprehensive Subset Testing
- **Multi-dimensional Operations**: 2D→1D, 3D→2D transformations
- **Negative Stride Support**: Reverse iteration with proper validation
- **Edge Case Coverage**: Boundary conditions and error scenarios
- **Performance Validation**: Large array slicing benchmarks

#### Factory Pattern Testing
- **Dynamic Loading**: Plugin loading and unloading scenarios
- **Configuration Validation**: TOML parsing and schema compliance
- **Error Handling**: Factory creation failure scenarios

### 2. Test Data Organization

```
test/
├── data/           # Test datasets
├── mappings/       # Test mapping configurations  
├── schemas/        # Schema validation tests
├── config/         # TOML configuration test files
└── src/            # Test implementations
    ├── subset_test.cpp      # Comprehensive subset operations
    ├── factory_test.cpp     # Data source factory tests
    └── config_test.cpp      # TOML configuration tests
```

## Future Directions

### 1. Scalability Enhancements

- Distributed caching
- Parallel data processing
- Stream processing support
- Cloud-native deployment

### 2. Developer Experience

- IDE integration
- Debug visualization tools
- Configuration validation IDE plugins
- Interactive mapping editor

### 3. Ecosystem Integration

- Python bindings
- REST API wrapper
- Configuration management tools
- Monitoring and observability