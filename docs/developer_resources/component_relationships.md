# LibTokaMap Component Relationships

This document provides detailed relationship diagrams and interaction patterns between components in the LibTokaMap library.

## Core Component Relationships

### 1. High-Level System Architecture

```mermaid
graph TB
    subgraph "Client Interface"
        API[Public API]
        CONFIG[Configuration]
    end
    
    subgraph "Core Engine"
        MH[MappingHandler]
        ER[ExperimentRegister]
        DSR[DataSourceRegistry]
    end
    
    subgraph "Mapping System"
        VM[ValueMapping]
        DSM[DataSourceMapping]
        EM[ExprMapping]
        DM[DimMapping]
        CM[CustomMapping]
    end
    
    subgraph "Data Layer"
        DS[DataSource]
        TDA[TypedDataArray]
        RC[RamCache]
    end
    
    subgraph "Utilities"
        SUBSET[Subset Operations]
        SCALE[Scale/Offset]
        RENDER[Template Rendering]
        VALID[Validation]
    end
    
    API --> MH
    CONFIG --> MH
    MH --> ER
    MH --> DSR
    MH --> VM
    MH --> DSM
    MH --> EM
    MH --> DM
    MH --> CM
    DSM --> DS
    DS --> TDA
    DSM --> RC
    DSM --> SUBSET
    DSM --> SCALE
    EM --> RENDER
    MH --> VALID
```

### 2. Mapping Handler Dependencies

```mermaid
graph LR
    subgraph "MappingHandler Core"
        MH[MappingHandler]
        INIT["init()"]
        RESET["reset()"]
        MAP["map()"]
        REG_DS["register_data_source()"]
        REG_CF["register_custom_function()"]
    end
    
    subgraph "Internal State"
        DSR[m_data_sources]
        ERS[m_experiment_register]
        RC[m_ram_cache]
        SCHEMAS[m_*_schema]
        LF[m_library_functions]
    end
    
    subgraph "External Dependencies"
        FS[Filesystem]
        JSON[nlohmann::json]
        VALIJSON[valijson]
        INJA[Inja]
    end
    
    MH --> DSR
    MH --> ERS
    MH --> RC
    MH --> SCHEMAS
    MH --> LF
    
    INIT --> FS
    INIT --> JSON
    INIT --> VALIJSON
    MAP --> INJA
    
    INIT -.-> DSR
    INIT -.-> ERS
    INIT -.-> RC
    MAP -.-> ERS
    REG_DS -.-> DSR
    REG_CF -.-> LF
```

### 3. Data Flow Through Mapping Types

```mermaid
sequenceDiagram
    participant Client
    participant MH as MappingHandler
    participant ER as ExperimentRegister
    participant M as Mapping
    participant DS as DataSource
    participant TDA as TypedDataArray
    participant Utils as Utilities
    
    Client->>MH: map(experiment, path, type, rank, attrs)
    MH->>ER: resolve mapping
    ER-->>MH: Mapping*
    
    alt ValueMapping
        MH->>M: map(args)
        M->>TDA: create from value
        TDA-->>M: result
    else DataSourceMapping
        MH->>M: map(args)
        M->>DS: get(args)
        DS-->>M: raw data
        M->>Utils: apply transformations
        Utils-->>M: transformed data
        M->>TDA: create result
        TDA-->>M: result
    else ExprMapping
        MH->>M: map(args)
        M->>Utils: evaluate expression
        Utils-->>M: computed value
        M->>TDA: create result
        TDA-->>M: result
    else CustomMapping
        MH->>M: map(args)
        M->>M: call library function
        M->>TDA: create result
        TDA-->>M: result
    end
    
    M-->>MH: TypedDataArray
    MH-->>Client: final result
```

## Detailed Component Interactions

### 1. TypedDataArray Lifecycle

```mermaid
stateDiagram-v2
    [*] --> Created: Constructor
    Created --> Populated: Data assignment
    Populated --> Transformed: apply() / slice()
    Transformed --> Accessed: data() / span()
    Accessed --> Moved: std#colon;#colon;move
    Moved --> [*]: Destructor
    
    Created --> [*]: Empty destructor
    Populated --> [*]: Owning destructor
    Transformed --> [*]: Owning destructor
    
    note right of Transformed
        Transformations create
        new owned buffer
    end note
    
    note right of Moved
        Original becomes
        non-owning
    end note
```

### 2. Experiment Loading Process

```mermaid
flowchart TD
    START["load_experiment()"] --> CHECK{Experiment loaded?}
    CHECK -->|Yes| RETURN[Return existing]
    CHECK -->|No| LOAD_CFG[Load mappings.cfg.json]
    LOAD_CFG --> VALIDATE_CFG[Validate config schema]
    VALIDATE_CFG --> LOAD_GLOBALS[Load top-level globals.json]
    LOAD_GLOBALS --> VALIDATE_GLOBALS[Validate globals schema]
    VALIDATE_GLOBALS --> ITERATE_GROUPS[Iterate groups]
    
    ITERATE_GROUPS --> SCAN_PARTITIONS[Scan partition directories]
    SCAN_PARTITIONS --> LOAD_PARTITION[Load partition mappings.json]
    LOAD_PARTITION --> VALIDATE_MAPPINGS[Validate mappings schema]
    VALIDATE_MAPPINGS --> PARSE_MAPPINGS[Parse mapping objects]
    PARSE_MAPPINGS --> STORE_MAPPINGS[Store in ExperimentMappings]
    
    STORE_MAPPINGS --> MORE_PARTITIONS{More partitions?}
    MORE_PARTITIONS -->|Yes| SCAN_PARTITIONS
    MORE_PARTITIONS -->|No| MORE_GROUPS{More groups?}
    MORE_GROUPS -->|Yes| ITERATE_GROUPS
    MORE_GROUPS -->|No| MARK_LOADED[Mark experiment as loaded]
    MARK_LOADED --> RETURN
```

### 3. Data Source Integration Pattern

```mermaid
classDiagram
    class DataSourceRegistry {
        -unordered_map~string,unique_ptr~DataSource~~ sources
        +register_source(name, source)
        +register_source(name, factory_name, args)
        +get_source(name) DataSource*
        +unregister_source(name)
    }
    
    class DataSourceFactory {
        <<function>>
        +create(args) unique_ptr~DataSource~
    }
    
    class DataSourceFactoryRegistry {
        -unordered_map~string,DataSourceFactory~ factories
        +register_factory(name, factory)
        +register_factory(name, library_path)
        +get_factory(name) DataSourceFactory
        +unregister_factory(name)
    }
    
    class DataSource {
        <<abstract>>
        +get(args, context, cache) TypedDataArray
    }
    
    class JSONDataSource {
        -filesystem::path root_path
        +get(args, context, cache) TypedDataArray
        -load_json_file(path) nlohmann::json
        -extract_data(json, path) TypedDataArray
    }
    
    class HDF5DataSource {
        -filesystem::path file_path
        +get(args, context, cache) TypedDataArray
        -read_hdf5_dataset(path) TypedDataArray
        -handle_hdf5_errors() void
    }
    
    class DataSourceMapping {
        -DataSource* data_source
        -DataSourceArgs args
        -optional~float~ scale
        -optional~float~ offset
        -optional~string~ slice
        +map(context) TypedDataArray
        -apply_transformations(data) TypedDataArray
    }
    
    DataSourceRegistry *-- DataSource
    DataSourceRegistry --> DataSourceFactoryRegistry : uses
    DataSourceFactoryRegistry *-- DataSourceFactory
    DataSourceFactory ..> DataSource : creates
    DataSource <|-- JSONDataSource
    DataSource <|-- HDF5DataSource
    DataSourceMapping --> DataSource
    DataSourceMapping ..> DataSourceRegistry : uses
```

### 4. Cache Integration

```mermaid
graph TB
    subgraph "Cache Architecture"
        RC[RamCache]
        CK[CacheKey]
        CE[CacheEntry]
        LRU[LRU Policy]
    end
    
    subgraph "Data Sources"
        DS[DataSource]
        DSM[DataSourceMapping]
    end
    
    subgraph "Cache Operations"
        GET["get()"]
        PUT["put()"]
        EVICT["evict()"]
        CLEAR["clear()"]
    end
    
    DSM -->|check cache| RC
    RC -->|cache miss| DS
    DS -->|raw data| DSM
    DSM -->|store result| RC
    
    RC --> CK
    RC --> CE
    RC --> LRU
    
    RC --> GET
    RC --> PUT
    RC --> EVICT
    RC --> CLEAR
    
    CK -.->|generates hash| CE
    LRU -.->|manages| CE
```

## Cross-Component Communication Patterns

### 1. Observer Pattern for Cache Events

```mermaid
sequenceDiagram
    participant DSM as DataSourceMapping
    participant RC as RamCache
    participant DS as DataSource
    participant LRU as LRUPolicy
    
    DSM->>RC: get(key)
    RC->>RC: lookup(key)
    
    alt Cache Hit
        RC-->>DSM: cached_data
    else Cache Miss
        RC->>DS: fetch_data()
        DS-->>RC: raw_data
        RC->>LRU: record_access(key)
        RC->>RC: store(key, data)
        
        alt Cache Full
            RC->>LRU: get_eviction_candidate()
            LRU-->>RC: old_key
            RC->>RC: evict(old_key)
        end
        
        RC-->>DSM: fresh_data
    end
```

### 2. Template Resolution Chain

```mermaid
flowchart LR
    subgraph "Template Context"
        GA[Global Attributes]
        LA[Local Attributes]
        RA[Runtime Attributes]
    end
    
    subgraph "Resolution Process"
        PARSE[Parse Template]
        RESOLVE[Resolve Variables]
        RENDER[Render Result]
    end
    
    subgraph "Variable Sources"
        GLOBALS[globals.json]
        PARTITION[partition globals]
        RUNTIME[extra_attributes]
        COMPUTED[computed values]
    end
    
    GLOBALS --> GA
    PARTITION --> LA
    RUNTIME --> RA
    
    GA --> RESOLVE
    LA --> RESOLVE
    RA --> RESOLVE
    COMPUTED --> RESOLVE
    
    PARSE --> RESOLVE
    RESOLVE --> RENDER
```

### 3. Error Propagation Chain

```mermaid
graph TB
    subgraph "Error Sources"
        JSON_ERR[JSON Parse Error]
        SCHEMA_ERR[Schema Validation Error]
        FILE_ERR[File System Error]
        TYPE_ERR[Type Mismatch Error]
        PARAM_ERR[Parameter Error]
    end
    
    subgraph "Exception Types"
        VALIDATION[ValidationError]
        DATA_SOURCE[DataSourceError]
        MAPPING[MappingError]
        PROCESSING[ProcessingError]
        DATA_TYPE[DataTypeError]
        PARAMETER[ParameterError]
    end
    
    subgraph "Error Handling"
        CATCH[Exception Handler]
        LOG[Error Logging]
        CLEANUP[Resource Cleanup]
        PROPAGATE[Propagate to Client]
    end
    
    JSON_ERR --> VALIDATION
    SCHEMA_ERR --> VALIDATION
    FILE_ERR --> DATA_SOURCE
    TYPE_ERR --> DATA_TYPE
    PARAM_ERR --> PARAMETER
    
    VALIDATION --> CATCH
    DATA_SOURCE --> CATCH
    MAPPING --> CATCH
    PROCESSING --> CATCH
    DATA_TYPE --> CATCH
    PARAMETER --> CATCH
    
    CATCH --> LOG
    CATCH --> CLEANUP
    CATCH --> PROPAGATE
```

## Resource Management Patterns

### 1. Memory Ownership Model

```mermaid
graph TD
    subgraph "Owned Resources"
        TDA_OWN["TypedDataArray (owning)"]
        BUFFER_MALLOC[malloc'd buffer]
        UNIQUE_PTR[unique_ptr wrappers]
    end
    
    subgraph "Borrowed Resources"
        TDA_REF["TypedDataArray (non-owning)"]
        RAW_PTR[raw pointers]
        SPAN_VIEW[span views]
    end
    
    subgraph "Shared Resources"
        CACHE["RamCache (shared_ptr)"]
        CONFIG[Configuration data]
        SCHEMAS[Validation schemas]
    end
    
    TDA_OWN --> BUFFER_MALLOC
    TDA_REF --> RAW_PTR
    UNIQUE_PTR -.-> TDA_OWN
    SPAN_VIEW -.-> TDA_REF
    
    CACHE -.->|reference counted| CONFIG
    SCHEMAS -.->|singleton pattern| CONFIG
```

### 2. Library Loading Lifecycle

```mermaid
stateDiagram-v2
    [*] --> Scanning: Scan library paths
    Scanning --> Loading: "dlopen()"
    Loading --> Resolving: "dlsym()"
    Resolving --> Validating: Check signature
    Validating --> Registered: Add to registry
    Registered --> InUse: Function calls
    InUse --> InUse: Multiple calls
    InUse --> Unloading: Explicit unregister
    Unloading --> [*]: "dlclose()"
    
    Loading --> Error: Load failure
    Resolving --> Error: Symbol not found
    Validating --> Error: Invalid signature
    Error --> [*]: Cleanup
```

### 3. Configuration Validation Pipeline

```mermaid
flowchart LR
    subgraph "Input Validation"
        TOML_PARSE[Parse TOML/JSON]
        SCHEMA_LOAD[Load Schema]
        VALIDATE[Validate Against Schema]
    end
    
    subgraph "Semantic Validation"
        REF_CHECK[Reference Checking]
        TYPE_CHECK[Type Compatibility]
        RANGE_CHECK[Range Validation]
        FACTORY_CHECK[Factory Validation]
    end
    
    subgraph "Runtime Validation"
        PATH_CHECK[Path Existence]
        PERM_CHECK[Permission Check]
        DEP_CHECK[Dependency Check]
        LIBRARY_CHECK[Library Loading Check]
    end
    
    TOML_PARSE --> SCHEMA_LOAD
    SCHEMA_LOAD --> VALIDATE
    VALIDATE --> REF_CHECK
    REF_CHECK --> TYPE_CHECK
    TYPE_CHECK --> RANGE_CHECK
    RANGE_CHECK --> FACTORY_CHECK
    FACTORY_CHECK --> PATH_CHECK
    PATH_CHECK --> PERM_CHECK
    PERM_CHECK --> DEP_CHECK
    DEP_CHECK --> LIBRARY_CHECK
    
    LIBRARY_CHECK --> SUCCESS[Configuration Valid]
    
    TOML_PARSE --> FAIL[Validation Failed]
    VALIDATE --> FAIL
    REF_CHECK --> FAIL
    TYPE_CHECK --> FAIL
    RANGE_CHECK --> FAIL
    FACTORY_CHECK --> FAIL
    PATH_CHECK --> FAIL
    PERM_CHECK --> FAIL
    DEP_CHECK --> FAIL
    LIBRARY_CHECK --> FAIL
```

## Enhanced Subset Operations

### 4. Advanced Slicing Architecture

```mermaid
stateDiagram-v2
    [*] --> ParseSlice: parse_slices(\"[#colon;][9]\", shape)
    ParseSlice --> ValidateSlice: SubsetInfo validation
    ValidateSlice --> NegativeStrideCheck: Check stride direction
    
    NegativeStrideCheck --> PositiveStride: stride > 0
    NegativeStrideCheck --> NegativeStride: stride < 0
    
    PositiveStride --> ComputeIndices: Standard indexing
    NegativeStride --> ReverseCompute: Reverse indexing
    
    ComputeIndices --> ApplySlice: generate_indices()
    ReverseCompute --> ApplySlice: handle wraparound
    
    ApplySlice --> UpdateShape: Dimension reduction
    UpdateShape --> [*]: Sliced TypedDataArray
    
    ValidateSlice --> [*]: ValidationError
    
    note right of NegativeStride
        Enhanced validation for
        negative strides with
        proper boundary checking
    end note
```

### 5. Factory Loading Lifecycle

```mermaid
sequenceDiagram
    participant Config as TOML Config
    participant MH as MappingHandler
    participant FR as FactoryRegistry
    participant Lib as Dynamic Library
    participant DS as DataSource
    
    Config->>MH: data_source_factories config
    MH->>FR: "register_factory(name, path)"
    FR->>Lib: "dlopen(library_path)"
    Lib-->>FR: library handle
    FR->>Lib: "dlsym(\"create_data_source\")"
    Lib-->>FR: factory function
    
    Config->>MH: data_sources config
    MH->>FR: "get_factory(factory_name)"
    FR-->>MH: factory function
    MH->>FR: "factory(args)"
    FR->>DS: create DataSource
    DS-->>MH: DataSource instance
    MH->>MH: "register_data_source(name, ds)"
    
    note over Lib: Hot-reloadable libraries with proper cleanup
```

This component relationship documentation provides a comprehensive view of how different parts of LibTokaMap interact, including the new factory pattern and enhanced subset operations, making it easier to understand the system for maintenance, debugging, and refactoring purposes.
