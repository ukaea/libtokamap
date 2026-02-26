# LibTokaMap Knowledge Graph

A comprehensive architectural reference and refactoring guide for the LibTokaMap library.

## Overview

This knowledge graph provides a complete understanding of the LibTokaMap library's architecture, components, relationships, and evolution path. It serves as the authoritative reference for:

- **System Architecture**: Understanding how components interact
- **API Patterns**: Best practices for library usage
- **Refactoring Strategy**: Modernization roadmap and migration paths
- **Development Guidance**: Design patterns and extension points

## Knowledge Graph Components

### 1. [Core Knowledge Graph](knowledge_graph.md)
The main architectural reference covering:
- **Component hierarchy and relationships**
- **Data flow and processing pipelines**
- **Directory structure and organization**
- **Key algorithms and utilities**
- **Extension points and plugin architecture**
- **Error handling and exception hierarchy**
- **Dependencies and external libraries**
- **Performance considerations**
- **Testing strategy**

### 2. [Component Relationships](component_relationships.md)
Detailed interaction patterns between components:
- **High-level system architecture diagrams**
- **Data flow sequences and state transitions**
- **Cross-component communication patterns**
- **Resource management and ownership models**
- **Configuration validation pipelines**

### 3. [API Patterns](api_patterns.md)
Comprehensive usage patterns and best practices:
- **Core API initialization and configuration**
- **Data source registration patterns**
- **Error handling strategies**
- **Performance optimization techniques**
- **Integration patterns (RAII, factories, observers)**
- **Testing patterns with mock objects**

### 4. [Refactoring Guide](refactoring_guide.md)
Modernization roadmap and migration strategy:
- **High-priority refactoring opportunities**
- **Type system modernization**
- **Error handling with std::expected**
- **Performance optimizations (SIMD, memory pools)**
- **API evolution strategy with versioning**
- **Migration timelines and compatibility matrix**

## Quick Start Guide

### For New Developers
1. Read the [Core Knowledge Graph](knowledge_graph.md) to understand the overall architecture
2. Study [API Patterns](api_patterns.md) for practical usage examples
3. Review [Component Relationships](component_relationships.md) for detailed interactions
4. Check the [Refactoring Guide](refactoring_guide.md) for modern best practices

### For Maintainers
1. Use the knowledge graph to understand impact of changes across components
2. Reference [Component Relationships](component_relationships.md) for debugging complex interactions
3. Follow [Refactoring Guide](refactoring_guide.md) for modernization efforts
4. Apply [API Patterns](api_patterns.md) for consistent design decisions

### For Contributors
1. Study the architecture in [Core Knowledge Graph](knowledge_graph.md)
2. Follow established patterns from [API Patterns](api_patterns.md)
3. Consider modernization opportunities from [Refactoring Guide](refactoring_guide.md)
4. Understand component interactions via [Component Relationships](component_relationships.md)

## Architecture Summary

LibTokaMap is a C++20 library for flexible data mapping and transformation with the following key characteristics:

### Core Components
- **MappingHandler**: Main orchestrator managing experiments, data sources, and caching
- **Mapping Types**: Value, DataSource, Expression, Dimension, and Custom mappings
- **TypedDataArray**: Type-safe, move-only data container with shape awareness
- **Data Sources**: Pluggable architecture for various data backends
- **Configuration System**: JSON-based with schema validation

### Key Design Patterns
- **Plugin Architecture**: Extensible data sources and custom mapping functions
- **Type Safety**: Strong typing with C++20 features and runtime type checking
- **Caching**: Multi-level caching strategy for performance optimization
- **Template-Based Processing**: Generic algorithms with type specialization

### Data Flow
1. **Configuration Loading**: JSON configs validated against schemas
2. **Experiment Resolution**: Hierarchical directory structure navigation
3. **Mapping Execution**: Type-specific data transformation pipelines
4. **Result Delivery**: Type-safe data containers with ownership semantics

## Usage Examples

### TOML Configuration (Preferred)
```cpp
#include <libtokamap.hpp>

// Modern TOML-based initialization
libtokamap::MappingHandler handler;
handler.init("/path/to/config.toml");

auto result = handler.map("EXPERIMENT", "path/to/data", 
                         std::type_index{typeid(double)}, 1, {});
```

### Factory-Based Data Sources
```cpp
// TOML config.toml:
// [data_source_factories]
// json_factory = "/path/to/libjson_source.so"
// 
// [data_sources.JSON]  
// factory = "json_factory"
// args.data_root = "/path/to/data"

// Factory function in external library
extern "C" std::unique_ptr<libtokamap::DataSource> create_json_source(
    const libtokamap::DataSourceFactoryArgs& args) {
    return std::make_unique<JSONDataSource>(args.at("data_root"));
}
```

### Enhanced Subset Operations
```cpp
// Multi-dimensional slicing with negative strides
auto result = handler.map("EXPERIMENT", "array[:][9]", type, 1, {}); // Column selection
auto reversed = handler.map("EXPERIMENT", "array[10:0:-1]", type, 1, {}); // Reverse

// Safe array operations
libtokamap::TypedDataArray original{data};
auto cloned = original.clone(); // New clone method
```

## Modernization Roadmap

The library is evolving toward modern C++20+ practices:

### Recently Completed ✅
- **Factory Pattern**: Dynamic data source loading with plugin architecture
- **TOML Configuration**: Modern configuration format with comprehensive validation
- **Enhanced Subset Operations**: Multi-dimensional slicing with negative stride support
- **Comprehensive Testing**: 998 new test cases for subset operations
- **Plugin Foundation**: Hot-loadable data source libraries

### Immediate Improvements (High Priority)
- **Type System**: Replace `std::type_index` with `DataType` enum
- **String Handling**: Adopt `std::format` for string operations
- **Template Constraints**: Add C++20 concepts for better error messages
- **Immutable Configuration**: Thread-safe configuration objects

### Medium-Term Enhancements
- **Error Handling**: Introduce `std::expected` alongside exceptions
- **Async Support**: Coroutine-based data loading
- **Performance**: SIMD optimizations and memory pools
- **Enhanced Plugin System**: Hot-reloading for development workflows

### Long-Term Vision
- **Functional Composition**: Pipeline-based mapping transformations
- **Distributed Computing**: Multi-node data processing
- **Real-time Streams**: Live data processing capabilities
- **Cloud Integration**: Native cloud storage and compute support

## Compatibility Strategy

The library maintains backward compatibility while introducing modern features:

- **Versioned APIs**: v1 (current) and v2 (modern) namespaces
- **Gradual Migration**: New features alongside existing APIs
- **Deprecation Timeline**: Clear migration paths with tooling support
- **Testing Strategy**: Comprehensive compatibility and performance testing

## Performance Characteristics

### Memory Management
- Move semantics for zero-copy data handling
- Optional memory pool allocation for high-frequency operations
- RAII-based resource management throughout

### Computational Efficiency
- Template specialization for common data types
- SIMD-optimized mathematical operations
- Multi-level caching with LRU eviction policies
- Lazy loading of configuration and data

### Scalability Considerations
- Thread-safe operations with minimal locking
- Lock-free data structures for high-contention scenarios
- Configurable cache sizes and eviction policies
- Plugin architecture for custom optimizations

## Extension Points

LibTokaMap provides several extension mechanisms:

### Data Sources
Implement the `DataSource` interface to support new data backends:
- File systems (JSON, HDF5, NetCDF, etc.)
- Databases (SQL, NoSQL, time-series)
- Network protocols (HTTP, gRPC, message queues)
- In-memory data structures

### Mapping Functions
Create custom mapping logic via external libraries:
- Mathematical transformations
- Statistical operations
- Domain-specific algorithms
- Integration with external tools

### Configuration
Extend the JSON schema for custom mapping types:
- New mapping categories
- Custom validation rules
- Domain-specific attributes
- Integration metadata

## Quality Assurance

### Testing Strategy
- **Unit Tests**: Individual component validation
- **Integration Tests**: Cross-component interaction testing
- **Property Tests**: Invariant verification with random inputs
- **Performance Tests**: Regression detection and optimization validation
- **Compatibility Tests**: API stability across versions

### Static Analysis
- **clang-tidy**: Modern C++ best practices enforcement
- **cppcheck**: Additional static analysis rules
- **Compiler Warnings**: Strict warning levels with -Werror
- **Sanitizers**: Runtime error detection during development

### Documentation
- **API Documentation**: Comprehensive function and class documentation
- **Architecture Guides**: High-level design documentation
- **Usage Examples**: Practical implementation patterns
- **Migration Tools**: Automated refactoring assistance

## Contributing to the Knowledge Graph

This knowledge graph is a living document that evolves with the codebase:

### Updating the Graph
1. **Architecture Changes**: Update component diagrams and relationships
2. **New Features**: Add API patterns and usage examples
3. **Performance Improvements**: Document optimizations and benchmarks
4. **Refactoring Progress**: Track modernization efforts and migration status

### Review Process
1. **Technical Review**: Architecture and implementation validation
2. **Documentation Review**: Clarity and completeness verification
3. **Testing Validation**: Ensure examples work with current codebase
4. **Community Feedback**: Incorporate user experience improvements

## Getting Help

### Documentation Resources
- **README.md**: Basic library overview and build instructions
- **API Reference**: Generated from source code comments
- **Examples**: Working code demonstrations in `examples/`
- **Knowledge Graph**: This comprehensive architectural reference

### Development Support
- **Issue Tracker**: Bug reports and feature requests
- **Discussion Forums**: Architecture and usage questions
- **Code Reviews**: Contribution feedback and guidance
- **Testing Infrastructure**: Automated validation and performance monitoring

---

*This knowledge graph represents the collective understanding of the LibTokaMap architecture as of the current version. It serves as both a reference for current usage and a roadmap for future development.*