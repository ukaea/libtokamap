# LibTokaMap Architectural Improvements

*Generated from knowledge graph analysis*

## Executive Summary

Based on systematic analysis of the LibTokaMap knowledge graph, this document outlines 12 architectural improvements prioritized by impact and feasibility. The analysis identified key areas for enhancement: component coupling, documentation gaps, type system modernization, and extensibility improvements.

### Key Findings

- **Highest Coupling**: MappingHandler (8 connections) and ValidationSchemas (7 connections) show concerning coupling levels
- **Documentation Gaps**: 4 of 5 core components lack dedicated documentation
- **Architecture Strengths**: No circular dependencies detected, clean inheritance hierarchies, well-defined extension points
- **Alignment with Roadmap**: Several improvements align with existing planned refactorings

---

## High Priority Improvements

### 1. Reduce MappingHandler Coupling ⚠️

**Category**: Architecture  
**Issue**: MappingHandler has 8 total connections - highest coupling in the system

**Current State**:
- Acts as orchestrator with too many direct responsibilities
- Manages experiments, data sources, caching, validation, and workflows
- Difficult to test in isolation
- Changes ripple through multiple concerns

**Benefits**:
- Improved testability through dependency injection
- Better separation of concerns
- Easier maintenance and evolution
- Reduced blast radius of changes

**Recommendations**:

1. **Create WorkflowCoordinator**
   ```cpp
   class WorkflowCoordinator {
   public:
       void executeInitialization(const Config& config);
       TypedDataArray executeMappingResolution(
           const MappingRequest& request
       );
   private:
       ExperimentRegister& m_experiments;
       DataSourceRegistry& m_data_sources;
       RamCache& m_cache;
   };
   ```

2. **Extract Validation Orchestration**
   - Move validation coordination to ValidationSchemas
   - MappingHandler should receive validated configs
   - Separate schema validation from runtime validation

3. **Apply Command Pattern**
   ```cpp
   class MappingCommand {
   public:
       virtual TypedDataArray execute() = 0;
       virtual bool canExecute() const = 0;
   };
   
   // MappingHandler becomes command executor
   class MappingHandler {
       TypedDataArray execute(std::unique_ptr<MappingCommand> cmd);
   };
   ```

4. **Introduce Mediator for Component Communication**
   - Reduce direct dependencies between components
   - Use event-based communication where appropriate

**Implementation Phases**:
1. Phase 1: Extract WorkflowCoordinator (low risk)
2. Phase 2: Refactor validation concerns (medium risk)
3. Phase 3: Introduce Command pattern (high impact)

---

### 2. Create Dedicated Configuration Layer

**Category**: Configuration  
**Issue**: Configuration concerns scattered across multiple components

**Current State**:
- ExperimentRegister manages experiment configs
- ValidationSchemas handles schema validation
- MappingHandler holds runtime config
- No centralized configuration authority
- Thread safety concerns with mutable configs

**Benefits**:
- Thread-safe immutable configuration objects
- Centralized validation pipeline
- Easier testing with mock configurations
- Support for configuration versioning and hot-reload

**Recommendations**:

1. **Create ConfigurationManager Component**
   ```cpp
   class ConfigurationManager {
   public:
       // Immutable configuration after initialization
       class Config {
       public:
           const ExperimentConfig& getExperiment(const std::string& name) const;
           const GlobalConfig& getGlobals() const;
           const DataSourceConfig& getDataSource(const std::string& name) const;
       private:
           // Immutable data members
       };
       
       // Builder for creating configs
       class ConfigBuilder {
       public:
           ConfigBuilder& loadFromFile(const std::filesystem::path& path);
           ConfigBuilder& validate();
           std::shared_ptr<const Config> build();
       };
       
       std::shared_ptr<const Config> loadConfiguration(
           const std::filesystem::path& config_path
       );
   };
   ```

2. **Implement Validation Pipeline**
   ```cpp
   class ValidationPipeline {
   public:
       ValidationPipeline& addRule(std::unique_ptr<ValidationRule> rule);
       Result<void, ValidationErrors> validate(const Config& config);
   };
   
   // Example usage
   ValidationPipeline pipeline;
   pipeline.addRule(std::make_unique<SchemaValidationRule>())
          .addRule(std::make_unique<CrossReferenceValidationRule>())
          .addRule(std::make_unique<DataSourceAvailabilityRule>());
   ```

3. **Support Configuration Contexts**
   - Global configuration (library-wide)
   - Experiment configuration (per-experiment)
   - Mapping configuration (per-mapping)
   - Runtime overrides (per-request)

4. **Enable Hot-Reload for Development**
   ```cpp
   class DevelopmentConfigManager : public ConfigurationManager {
       void watchForChanges(std::function<void(const Config&)> callback);
       void reload();
   };
   ```

**Migration Path**:
1. Create new ConfigurationManager alongside existing code
2. Migrate ExperimentRegister to use ConfigurationManager
3. Update ValidationSchemas to work with new pipeline
4. Deprecate old configuration APIs with clear migration guide

---

### 3. Document Internal Components

**Category**: Documentation  
**Issue**: 4 of 5 core components lack documentation

**Current State**:
- Only MappingHandler (API entry point) has documentation
- ExperimentRegister, DataSourceRegistry, RamCache, ValidationSchemas undocumented
- Internal architecture knowledge exists only in code
- High barrier to contribution and maintenance

**Benefits**:
- Easier onboarding for new developers
- Better maintainability and debugging
- Clear component responsibilities
- Reduced knowledge silos

**Recommendations**:

1. **Create `docs/internal-architecture.md`**
   ```markdown
   # Internal Architecture
   
   ## Component Overview
   - ExperimentRegister: Manages experiment configurations...
   - DataSourceRegistry: Registry pattern for data sources...
   - RamCache: LRU cache implementation...
   - ValidationSchemas: JSON schema validation...
   
   ## Component Interactions
   [Include sequence diagrams]
   
   ## Design Decisions
   - Why LRU for caching?
   - Why registry pattern for data sources?
   ```

2. **Add Component-Specific Documentation**
   - `docs/components/experiment-register.md`
   - `docs/components/data-source-registry.md`
   - `docs/components/ram-cache.md`
   - `docs/components/validation-schemas.md`

3. **Create Sequence Diagrams**
   ```mermaid
   sequenceDiagram
       participant Client
       participant MH as MappingHandler
       participant ER as ExperimentRegister
       participant DSR as DataSourceRegistry
       
       Client->>MH: map(experiment, path)
       MH->>ER: getExperiment(name)
       ER-->>MH: ExperimentMappings
       MH->>DSR: getDataSource(type)
       DSR-->>MH: DataSource*
   ```

4. **Enhance Inline Documentation**
   ```cpp
   /**
    * @brief Manages experiment configurations and partition selection
    * 
    * ExperimentRegister maintains a hierarchical structure of experiments,
    * groups, and partitions. It handles dynamic partition selection based
    * on attribute values using configurable selection strategies.
    * 
    * Thread Safety: Read operations are thread-safe after initialization.
    * Write operations must be synchronized externally.
    * 
    * @see MappingPartition for partition selection strategies
    * @see DirectorySelector for selection algorithms
    */
   class ExperimentRegister {
       // ...
   };
   ```

**Content Requirements**:
- Purpose and responsibilities
- Public API with examples
- Internal architecture and data structures
- Thread safety guarantees
- Performance characteristics
- Common usage patterns
- Troubleshooting guide

---

### 4. Add Data Type Registry

**Category**: Type System  
**Issue**: Type handling uses `std::type_index` which is not extensible  
**Aligns With**: Type System Modernization refactoring

**Current State**:
- Uses `std::type_index` for runtime type identification
- Limited to built-in C++ types
- No support for custom or domain-specific types
- Poor error messages for type mismatches
- Cannot serialize/deserialize type information

**Benefits**:
- Extensible type system for custom types
- Better error messages with type names
- Support for type conversions and coercion
- Serialization support for type metadata
- Domain-specific type hierarchies

**Recommendations**:

1. **Create DataType Enum and Registry**
   ```cpp
   enum class DataType {
       Float32,
       Float64,
       Int32,
       Int64,
       String,
       Bool,
       Custom  // For user-defined types
   };
   
   class TypeRegistry {
   public:
       static TypeRegistry& instance();
       
       // Register custom type
       template<typename T>
       void registerType(const std::string& name, DataType base = DataType::Custom);
       
       // Query type information
       DataType getTypeFromIndex(std::type_index idx) const;
       std::string getTypeName(DataType type) const;
       
       // Type conversions
       bool canConvert(DataType from, DataType to) const;
       TypedDataArray convert(const TypedDataArray& data, DataType target);
       
   private:
       std::unordered_map<std::type_index, DataType> m_type_map;
       std::unordered_map<DataType, TypeInfo> m_type_info;
   };
   ```

2. **Update TypedDataArray**
   ```cpp
   class TypedDataArray {
   public:
       DataType getDataType() const { return m_type; }
       std::string getTypeName() const { 
           return TypeRegistry::instance().getTypeName(m_type); 
       }
       
       // Type-safe conversion
       template<typename T>
       std::optional<T> getAs() const;
       
   private:
       DataType m_type;
       std::variant</* ... */> m_data;
   };
   ```

3. **Enhanced Error Messages**
   ```cpp
   // Before
   throw MappingError("Type mismatch");
   
   // After
   throw MappingError(std::format(
       "Type mismatch: expected {}, got {}",
       TypeRegistry::instance().getTypeName(expected),
       TypeRegistry::instance().getTypeName(actual)
   ));
   ```

4. **Support Type Metadata**
   ```cpp
   struct TypeInfo {
       std::string name;
       size_t size;
       std::function<TypedDataArray(const std::string&)> fromString;
       std::function<std::string(const TypedDataArray&)> toString;
       std::optional<DataType> baseType;
   };
   ```

---

## Medium Priority Improvements

### 5. Add Observable/Event System

**Category**: Extensibility  
**Issue**: No event system for monitoring data flow and transformations

**Benefits**:
- Better debugging and diagnostics
- Performance monitoring hooks
- Plugin integration points
- Audit logging capabilities

**Recommendations**:

1. **Define Event Types**
   ```cpp
   enum class MappingEvent {
       MappingStarted,
       MappingCompleted,
       MappingFailed,
       CacheHit,
       CacheMiss,
       DataSourceAccessed,
       ValidationFailed
   };
   
   struct EventData {
       MappingEvent type;
       std::string mapping_name;
       std::chrono::milliseconds duration;
       std::optional<std::string> error_message;
   };
   ```

2. **Implement Observer Pattern**
   ```cpp
   class MappingObserver {
   public:
       virtual void onEvent(const EventData& event) = 0;
       virtual ~MappingObserver() = default;
   };
   
   class MappingHandler {
   public:
       void addObserver(std::shared_ptr<MappingObserver> observer);
       void removeObserver(std::shared_ptr<MappingObserver> observer);
       
   private:
       void notifyObservers(const EventData& event);
       std::vector<std::shared_ptr<MappingObserver>> m_observers;
   };
   ```

3. **Built-in Observers**
   ```cpp
   // Logging observer
   class LoggingObserver : public MappingObserver {
       void onEvent(const EventData& event) override;
   };
   
   // Metrics observer
   class MetricsObserver : public MappingObserver {
       void onEvent(const EventData& event) override;
   private:
       std::unordered_map<std::string, Statistics> m_stats;
   };
   
   // Debugging observer
   class DebugObserver : public MappingObserver {
       void onEvent(const EventData& event) override;
   private:
       std::vector<EventData> m_history;
   };
   ```

---

### 6. Implement Result Type Pattern

**Category**: Error Handling  
**Issue**: Error handling relies solely on exceptions  
**Aligns With**: Error Handling with std::expected refactoring

**Benefits**:
- Explicit error handling in function signatures
- Better performance (no exception overhead)
- Clearer API contracts
- Support for functional composition

**Recommendations**:

1. **Introduce Result Type**
   ```cpp
   template<typename T, typename E = Error>
   class Result {
   public:
       static Result success(T value);
       static Result failure(E error);
       
       bool isSuccess() const;
       bool isFailure() const;
       
       const T& value() const;
       const E& error() const;
       
       // Monadic operations
       template<typename F>
       auto map(F&& f) -> Result<decltype(f(std::declval<T>())), E>;
       
       template<typename F>
       auto flatMap(F&& f) -> decltype(f(std::declval<T>()));
       
       T valueOr(T default_value) const;
   };
   ```

2. **Define Error Hierarchy**
   ```cpp
   struct Error {
       std::string message;
       ErrorCategory category;
       std::optional<std::string> context;
       
       enum class ErrorCategory {
           FileNotFound,
           ValidationFailed,
           DataSourceError,
           MappingError,
           ConfigurationError
       };
   };
   ```

3. **Dual API Approach**
   ```cpp
   // Exception-based (backwards compatible)
   TypedDataArray map(const std::string& experiment, 
                      const std::string& path);
   
   // Result-based (new, preferred)
   Result<TypedDataArray, Error> tryMap(
       const std::string& experiment,
       const std::string& path
   );
   ```

4. **Error Composition**
   ```cpp
   Result<TypedDataArray, Error> processMapping() {
       return loadConfig()
           .flatMap([](const Config& cfg) { return validateConfig(cfg); })
           .flatMap([](const Config& cfg) { return executeMapping(cfg); })
           .map([](TypedDataArray data) { return applyTransforms(data); });
   }
   ```

---

### 7. Separate Validation Concerns

**Category**: Architecture  
**Issue**: ValidationSchemas has 7 connections - second highest coupling

**Recommendations**:

1. **Split into SchemaValidator and RuntimeValidator**
   ```cpp
   class SchemaValidator {
   public:
       Result<void, ValidationErrors> validateAgainstSchema(
           const nlohmann::json& data,
           const Schema& schema
       );
   };
   
   class RuntimeValidator {
   public:
       Result<void, ValidationErrors> validateDataConsistency(
           const TypedDataArray& data,
           const ValidationRules& rules
       );
   };
   ```

2. **Pluggable Validation Rules**
   ```cpp
   class ValidationRule {
   public:
       virtual Result<void, ValidationError> validate(
           const ValidationContext& context
       ) = 0;
       virtual std::string getName() const = 0;
   };
   
   // Examples
   class DataSourceExistsRule : public ValidationRule { /* ... */ };
   class TypeCompatibilityRule : public ValidationRule { /* ... */ };
   class RangeCheckRule : public ValidationRule { /* ... */ };
   ```

---

### 8. Create Mapping Pipeline Builder

**Category**: API Improvement  
**Issue**: No fluent API for building complex mapping pipelines

**Recommendations**:

```cpp
class MappingPipeline {
public:
    class Builder {
    public:
        Builder& fromMapping(const std::string& name);
        Builder& transform(std::function<TypedDataArray(TypedDataArray)> fn);
        Builder& filter(std::function<bool(const TypedDataArray&)> predicate);
        Builder& cache(bool enabled = true);
        Builder& withTimeout(std::chrono::milliseconds timeout);
        Builder& onError(std::function<void(const Error&)> handler);
        
        MappingPipeline build();
    };
    
    Result<TypedDataArray, Error> execute(
        MappingHandler& handler,
        const MapArguments& args
    );
};

// Usage
auto pipeline = MappingPipeline::Builder()
    .fromMapping("temperature_data")
    .transform([](auto data) { return convertUnits(data); })
    .filter([](const auto& data) { return isValid(data); })
    .cache(true)
    .withTimeout(std::chrono::seconds(5))
    .build();

auto result = pipeline.execute(handler, args);
```

---

### 9. Implement Async Data Loading

**Category**: Performance  
**Issue**: No asynchronous data loading support  
**Aligns With**: Async Support refactoring

**Recommendations**:

```cpp
// Async data source interface
class AsyncDataSource {
public:
    virtual std::future<TypedDataArray> getAsync(
        const DataSourceArgs& args,
        const MapArguments& arguments,
        RamCache* cache
    ) = 0;
};

// Coroutine-based API (C++20)
class DataSourceMapping {
public:
    std::future<TypedDataArray> mapAsync(const MapArguments& args) {
        co_await loadDataAsync();
        co_await applyTransformations();
        co_return result;
    }
};

// Parallel mapping execution
class MappingHandler {
public:
    std::future<std::vector<TypedDataArray>> mapAll(
        const std::vector<MappingRequest>& requests
    ) {
        std::vector<std::future<TypedDataArray>> futures;
        for (const auto& req : requests) {
            futures.push_back(std::async([&] { return map(req); }));
        }
        co_return collectResults(futures);
    }
};
```

---

### 10. Add Configuration Schema Extension Documentation

**Category**: Documentation  
**Issue**: Configuration Schema extension point has no documentation  
**Addresses**: Configuration Schema extension point

**Recommendations**:

Create `docs/extending-configuration.md`:
- How to define custom mapping types
- Schema composition patterns
- Validation rule examples
- Best practices for schema design

---

## Low Priority Improvements

### 11. Add Caching Strategy Interface

**Category**: Extensibility  
**Issue**: RamCache is hard-coded LRU implementation

**Recommendations**:

```cpp
class CacheStrategy {
public:
    virtual void put(const std::string& key, const TypedDataArray& value) = 0;
    virtual std::optional<TypedDataArray> get(const std::string& key) = 0;
    virtual void evict() = 0;  // Strategy-specific eviction
};

// Implementations
class LRUCache : public CacheStrategy { /* ... */ };
class LFUCache : public CacheStrategy { /* ... */ };
class FIFOCache : public CacheStrategy { /* ... */ };
class RedisCache : public CacheStrategy { /* ... */ };

// Configuration
config.cache_strategy = "LRU";
config.cache_params = {{"max_size", 100}};
```

---

### 12. Add Telemetry and Metrics Layer

**Category**: Observability  
**Issue**: No built-in metrics or performance monitoring

**Recommendations**:

```cpp
class MetricsCollector {
public:
    void recordMappingDuration(const std::string& name, 
                               std::chrono::milliseconds duration);
    void recordCacheHitRate(double rate);
    void recordDataSourceLatency(const std::string& source,
                                 std::chrono::milliseconds latency);
    
    MetricsSummary getSummary() const;
    void exportToPrometheus(std::ostream& out) const;
};

// Optional OpenTelemetry integration
class OpenTelemetryIntegration {
public:
    void configure(const std::string& endpoint);
    void exportTraces();
    void exportMetrics();
};
```

---

## Implementation Roadmap

### Phase 1: Foundation (Q1)
1. Document Internal Components (high priority, low risk)
2. Add Data Type Registry (high priority, aligns with refactoring)
3. Add Configuration Schema Extension Documentation (medium priority, low risk)

### Phase 2: Architecture (Q2)
1. Create Dedicated Configuration Layer (high priority, medium risk)
2. Reduce MappingHandler Coupling (high priority, medium risk)
3. Separate Validation Concerns (medium priority, medium risk)

### Phase 3: API Enhancement (Q3)
1. Implement Result Type Pattern (medium priority, high impact)
2. Create Mapping Pipeline Builder (medium priority, high value)
3. Add Observable/Event System (medium priority, high extensibility)

### Phase 4: Performance & Observability (Q4)
1. Implement Async Data Loading (medium priority, high performance impact)
2. Add Caching Strategy Interface (low priority, nice to have)
3. Add Telemetry and Metrics Layer (low priority, production value)

---

## Success Metrics

### Code Quality
- Reduce average component coupling from 5 to 3
- Achieve 90% documentation coverage for core components
- Zero circular dependencies (maintained)

### Developer Experience
- Reduce onboarding time from 2 weeks to 1 week
- Increase external contributions by 50%
- Achieve 4.5+ satisfaction rating from developers

### Performance
- Support async operations with 10x throughput improvement
- Reduce error handling overhead by 20% with Result types
- Enable parallel mapping execution

### Maintainability
- Reduce time to implement new features by 30%
- Improve test coverage to 85%
- Enable hot-reload for development workflows

---

## Conclusion

This architectural analysis reveals a well-designed system with clear extension points and no critical flaws. The suggested improvements focus on:

1. **Reducing complexity** through better separation of concerns
2. **Improving documentation** for internal components
3. **Modernizing patterns** with Result types and async support
4. **Enhancing extensibility** with events and pluggable strategies

The roadmap prioritizes high-impact, low-risk improvements first, allowing for iterative enhancement while maintaining backward compatibility. Many improvements align with existing refactoring plans, creating natural synergies.

The key insight from the graph analysis is that LibTokaMap has solid architectural foundations but would benefit significantly from better documentation, reduced coupling in orchestrator components, and modern C++20 patterns for error handling and async operations.

---

*This document was generated through systematic analysis of the LibTokaMap knowledge graph, examining component relationships, coupling metrics, documentation coverage, and architectural patterns.*