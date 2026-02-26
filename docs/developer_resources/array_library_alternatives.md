# Array Library Alternatives for TypedDataArray

## Executive Summary

LibTokaMap currently implements custom array slicing and subsetting in `TypedDataArray`. This analysis evaluates existing C++ array libraries that could replace or complement this implementation, weighing benefits, costs, and integration complexity.

**Recommendation**: Consider **xtensor** for new development while maintaining backward compatibility, or adopt **std::mdspan** (C++23) for a lightweight standard solution.

---

## Current Implementation: TypedDataArray

### Strengths
- ✅ **Custom-tailored** to LibTokaMap's needs
- ✅ **Type-safe** with variant-based storage
- ✅ **Move semantics** for zero-copy operations
- ✅ **Direct integration** with mapping system
- ✅ **Full control** over behavior and optimizations

### Weaknesses
- ❌ **Maintenance burden** - Complex slicing logic to maintain
- ❌ **Limited features** - No broadcasting, lazy evaluation, or advanced operations
- ❌ **Testing overhead** - Extensive edge case testing required (78 new tests added)
- ❌ **Reinventing the wheel** - Duplicating well-tested functionality
- ❌ **Missing optimizations** - No SIMD, no expression templates

### Current Capabilities
- Multi-dimensional arrays (1D, 2D, 3D+)
- Python-style slicing: `[start:stop:stride]`
- Negative indices and strides
- Scale/offset transformations
- Shape tracking and rank reduction
- Move-only semantics
- Variant-based type storage (`float`, `double`, `int`, `string`)

---

## Alternative Library Analysis

### 1. xtensor ⭐ **RECOMMENDED**

**Website**: https://github.com/xtensor-stack/xtensor  
**License**: BSD-3-Clause  
**C++ Standard**: C++14 (C++20 compatible)

#### Overview
NumPy-style multi-dimensional arrays for C++. Most feature-complete and mature option.

#### Features
- ✅ **NumPy-compatible API** - Easy mental model, extensive docs
- ✅ **Lazy evaluation** - Expression templates for efficiency
- ✅ **Broadcasting** - Automatic shape matching
- ✅ **Python/R/Julia bindings** - Interoperability via xtensor-python
- ✅ **Slicing syntax**: `xt::range(start, stop, stride)`
- ✅ **Universal functions** - SIMD-optimized operations
- ✅ **Header-only** option available
- ✅ **Active development** - Large community, regular updates
- ✅ **Zero-copy views** - Efficient memory usage

#### Slicing Comparison

**NumPy/Current**:
```python
arr[2:8:2]      # Python
arr[::-1]       # Reverse
arr[:-2]        # Negative indices
```

**xtensor**:
```cpp
// Direct slicing
auto view = xt::view(arr, xt::range(2, 8, 2));

// Reverse
auto reversed = xt::view(arr, xt::range(_, _, -1));

// Negative indices supported via normalization
auto slice = xt::view(arr, xt::range(_, -2));

// Strided view
auto strided = xt::strided_view(arr, {xt::ellipsis(), 3});
```

#### Integration Example

```cpp
#include <xtensor/xarray.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xadapt.hpp>

class TypedDataArray {
public:
    // Adapt existing data to xtensor (zero-copy)
    template<typename T>
    auto as_xtensor() {
        auto data_ptr = std::get<std::vector<T>>(m_data).data();
        return xt::adapt(data_ptr, m_shape);
    }
    
    // Slice using xtensor
    template<typename T>
    TypedDataArray slice(const std::string& slice_str) {
        auto xarr = as_xtensor<T>();
        auto view = parse_and_apply_slice(xarr, slice_str);
        
        // Convert back to TypedDataArray
        std::vector<T> result(view.begin(), view.end());
        return TypedDataArray{std::move(result), view.shape()};
    }
};
```

#### Pros
- **Mature and battle-tested** - Used in production by many projects
- **Feature-rich** - Broadcasting, lazy eval, SIMD, reducers
- **Excellent documentation** - Comprehensive guides and examples
- **NumPy familiarity** - Easy for Python users to understand
- **Performance** - Expression templates minimize temporaries
- **Ecosystem** - xtensor-blas, xtensor-fftw, xtensor-io

#### Cons
- **Learning curve** - Template metaprogramming can be complex
- **Compile times** - Heavy template usage increases compilation
- **API surface** - Large library might be overkill for basic slicing
- **Syntax verbosity** - More verbose than Python for simple operations
- **Not standard** - External dependency

#### Migration Complexity
- **Low-Medium**: Can be adopted incrementally
- **Strategy**: Wrap xtensor arrays in TypedDataArray interface
- **Backward compatibility**: Maintain existing API, use xtensor internally

---

### 2. std::mdspan (C++23) ⭐ **LIGHTWEIGHT STANDARD**

**Standard**: C++23  
**Reference**: https://en.cppreference.com/w/cpp/container/mdspan  
**Backport**: https://github.com/kokkos/mdspan (C++17 compatible)

#### Overview
Non-owning multi-dimensional view into contiguous memory. Part of C++ standard library.

#### Features
- ✅ **Standard library** - No external dependencies (in C++23)
- ✅ **Lightweight** - Minimal overhead, header-only backport
- ✅ **Non-owning views** - Doesn't manage memory
- ✅ **Static/dynamic extents** - Compile-time or runtime dimensions
- ✅ **Layout control** - Row-major, column-major, strided
- ✅ **Accessor policy** - Custom element access patterns
- ❌ **No built-in slicing** - Must implement or use submdspan (C++26)
- ❌ **No broadcasting** - Not included
- ❌ **No lazy evaluation** - Simple views only

#### Usage Example

```cpp
#include <mdspan>  // C++23
#include <vector>

std::vector<float> data = {1, 2, 3, 4, 5, 6};
std::mdspan<float, std::extents<size_t, 2, 3>> matrix(data.data());

// Access
float val = matrix[1, 2];  // Row 1, col 2

// Subspan (C++26 proposal, not yet standard)
// auto subview = std::submdspan(matrix, 0, std::full_extent);
```

#### Integration Strategy

```cpp
class TypedDataArray {
    std::variant<
        std::vector<float>,
        std::vector<double>,
        std::vector<int>
    > m_data;
    std::vector<size_t> m_shape;
    
public:
    // Create mdspan view
    template<typename T>
    auto as_mdspan() {
        auto& vec = std::get<std::vector<T>>(m_data);
        return std::mdspan(vec.data(), m_shape[0], m_shape[1]);
    }
    
    // Custom slicing on top of mdspan
    template<typename T>
    TypedDataArray slice(const SubsetInfo& subset) {
        auto span = as_mdspan<T>();
        // Implement custom slicing logic
        // Extract data into new TypedDataArray
    }
};
```

#### Pros
- **Standard library** - Part of C++ (C++23)
- **Zero dependencies** - Eventually no external libs needed
- **Minimal overhead** - Very lightweight
- **Flexible** - Custom layouts and accessors
- **Backport available** - Can use now with C++17+

#### Cons
- **No slicing built-in** - Must implement yourself (defeats purpose)
- **C++23 requirement** - Not widely available yet
- **Limited features** - Just views, no operations
- **No broadcasting** - Would need to add
- **Immature ecosystem** - Few helper libraries

#### Migration Complexity
- **Medium-High**: Slicing still needs implementation
- **Strategy**: Use mdspan as view layer, keep slicing logic
- **Value**: Mostly code organization, not feature gain

---

### 3. Eigen

**Website**: https://eigen.tuxfamily.org/  
**License**: MPL2  
**C++ Standard**: C++14

#### Overview
Powerful linear algebra library. Industry standard for matrix operations.

#### Features
- ✅ **Mature and optimized** - Highly performant
- ✅ **SIMD support** - Vectorization across platforms
- ✅ **Lazy evaluation** - Expression templates
- ✅ **Block operations** - Efficient submatrix access
- ✅ **Wide adoption** - Used in robotics, graphics, ML
- ❌ **Limited to 2D** - Primarily matrices (can be extended)
- ❌ **Different paradigm** - Linear algebra focus, not general arrays
- ❌ **No Python-style slicing** - Block-based API instead

#### Usage Example

```cpp
#include <Eigen/Dense>

Eigen::MatrixXd mat(10, 15);
mat.setRandom();

// Slicing (block-based)
auto sub = mat.block(2, 3, 5, 7);  // Start row, col, num rows, cols
auto row = mat.row(5);
auto col = mat.col(3);

// Reverse not directly supported
auto reversed = mat.colwise().reverse();
```

#### Pros
- **Extremely optimized** - Best performance for linear algebra
- **Battle-tested** - Used in production everywhere
- **Rich operations** - SVD, eigenvalues, solvers, etc.

#### Cons
- **Not designed for this** - Doesn't fit LibTokaMap's use case
- **2D focus** - Not ideal for 3D+ arrays
- **Different mental model** - Block-based, not slice-based
- **Heavy for simple slicing** - Overkill

#### Verdict
❌ **Not recommended** - Wrong tool for the job. Great for linear algebra, but LibTokaMap needs general N-D array slicing, not matrix operations.

---

### 4. Armadillo

**Website**: http://arma.sourceforge.net/  
**License**: Apache 2.0  
**C++ Standard**: C++11

#### Overview
MATLAB-like syntax for linear algebra. Similar to Eigen but different API.

#### Features
- Similar to Eigen (linear algebra focus)
- MATLAB-style API
- Limited to 2D/3D

#### Verdict
❌ **Not recommended** - Same issues as Eigen. Linear algebra focus doesn't match LibTokaMap's array manipulation needs.

---

### 5. Boost.MultiArray

**Website**: https://www.boost.org/doc/libs/1_84_0/libs/multi_array/  
**License**: Boost Software License  
**C++ Standard**: C++11

#### Overview
Older Boost library for multi-dimensional arrays.

#### Features
- ✅ **N-dimensional** - True multi-dimensional support
- ✅ **Slicing support** - Built-in slicing operations
- ✅ **Part of Boost** - May already be a dependency
- ❌ **Older design** - Pre-modern C++
- ❌ **Less active** - Maintenance mode
- ❌ **Verbose API** - Not as clean as modern alternatives
- ❌ **Limited optimization** - No expression templates

#### Verdict
⚠️ **Not recommended** - Superseded by modern alternatives like xtensor. If already using Boost extensively, might be okay, but xtensor is better.

---

## Comparison Matrix

| Feature | Current (TypedDataArray) | xtensor | std::mdspan | Eigen | Boost.MultiArray |
|---------|-------------------------|---------|-------------|-------|------------------|
| **N-D Arrays** | ✅ | ✅ | ✅ | ⚠️ (2D) | ✅ |
| **Python-style slicing** | ✅ | ✅ | ❌ | ❌ | ⚠️ |
| **Negative indices** | ✅ | ✅ | ❌ | ❌ | ❌ |
| **Negative strides** | ✅ | ✅ | ❌ | ❌ | ⚠️ |
| **Broadcasting** | ❌ | ✅ | ❌ | ⚠️ | ❌ |
| **Lazy evaluation** | ❌ | ✅ | ❌ | ✅ | ❌ |
| **SIMD optimization** | ❌ | ✅ | ❌ | ✅ | ❌ |
| **Move semantics** | ✅ | ✅ | ✅ | ✅ | ⚠️ |
| **Type variants** | ✅ | ⚠️ | ⚠️ | ❌ | ⚠️ |
| **Standard library** | N/A | ❌ | ✅ (C++23) | ❌ | ❌ |
| **Zero dependencies** | ✅ | ❌ | ✅ (C++23) | ❌ | ❌ |
| **Maturity** | Custom | High | Low | Very High | Medium |
| **Documentation** | Internal | Excellent | Good | Excellent | Good |
| **Community** | N/A | Large | Growing | Very Large | Medium |
| **Compile time** | Fast | Slow | Fast | Medium | Medium |
| **Learning curve** | N/A | Medium | Low | High | Medium |

---

## Recommendation Strategy

### Option A: Adopt xtensor (Recommended for Feature-Rich Solution)

**Use Case**: Need advanced features (broadcasting, lazy eval, SIMD)

**Implementation Strategy**:
1. **Phase 1**: Internal migration
   - Wrap xtensor arrays with existing TypedDataArray API
   - Maintain backward compatibility
   - Migrate slicing logic to use xtensor views
   
2. **Phase 2**: Expose new features (optional)
   - Add broadcasting support
   - Enable lazy evaluation
   - Expose xtensor operations
   
3. **Phase 3**: Full adoption
   - Make xtensor the primary implementation
   - Deprecate old TypedDataArray internals

**Code Example**:
```cpp
// Backward-compatible wrapper
class TypedDataArray {
    std::variant<
        xt::xarray<float>,
        xt::xarray<double>,
        xt::xarray<int>
    > m_array;
    
public:
    // Existing API maintained
    void slice(const std::string& slice_str) {
        std::visit([&](auto& arr) {
            auto view = parse_slice_to_xtensor(slice_str, arr);
            // Update internal state
        }, m_array);
    }
    
    // New API (optional)
    template<typename T>
    xt::xarray<T>& as_xtensor() {
        return std::get<xt::xarray<T>>(m_array);
    }
};
```

**Pros**:
- Gain extensive features with minimal code
- Battle-tested slicing implementation
- Performance improvements via SIMD
- Future-proof with active development

**Cons**:
- External dependency
- Increased compile times
- Learning curve for contributors

**Estimated Effort**: 2-3 weeks for core migration

---

### Option B: Adopt std::mdspan (Recommended for Minimal Dependencies)

**Use Case**: Want standard library solution, willing to keep slicing logic

**Implementation Strategy**:
1. Use mdspan for views/accessors
2. Keep existing slicing parser and logic
3. Refactor data storage to work with mdspan
4. Gradually adopt C++23 features

**Code Example**:
```cpp
class TypedDataArray {
    std::variant<
        std::vector<float>,
        std::vector<double>,
        std::vector<int>
    > m_data;
    std::vector<size_t> m_shape;
    
public:
    // Create view without copying
    template<typename T>
    auto view() {
        auto& vec = std::get<std::vector<T>>(m_data);
        return create_mdspan(vec.data(), m_shape);
    }
    
    // Slicing still uses existing logic
    void slice(const SubsetInfo& info) {
        // Current implementation stays
    }
};
```

**Pros**:
- Standard library (eventually)
- Minimal overhead
- Clean separation: view vs storage
- No external dependencies (long-term)

**Cons**:
- C++23 requirement (or backport)
- Still maintain slicing logic
- Limited feature gain
- Immature ecosystem

**Estimated Effort**: 1-2 weeks for integration

---

### Option C: Keep Current Implementation (Status Quo)

**Use Case**: Current solution works, avoid risk and churn

**Recommendation**: Enhance current implementation instead

**Improvements**:
1. ✅ **Add comprehensive tests** (already done - 78 new edge cases)
2. Add SIMD optimizations for hot paths
3. Add expression templates for chained operations
4. Improve error messages
5. Add fuzzing tests
6. Document edge cases thoroughly

**Pros**:
- No migration risk
- Full control over behavior
- No new dependencies
- Fast compile times
- Team already understands it

**Cons**:
- Ongoing maintenance burden
- Missing advanced features
- No broadcasting or lazy eval
- Reinventing optimizations

**Estimated Effort**: Ongoing maintenance

---

## Decision Matrix

| Criteria | Weight | Current | xtensor | mdspan | Keep Current |
|----------|--------|---------|---------|--------|--------------|
| **Feature completeness** | 20% | 60 | 95 | 40 | 60 |
| **Performance** | 20% | 70 | 95 | 85 | 75 |
| **Maintenance burden** | 15% | 40 | 85 | 70 | 50 |
| **Integration complexity** | 15% | 100 | 60 | 70 | 100 |
| **Dependencies** | 10% | 100 | 50 | 90 | 100 |
| **Community support** | 10% | 0 | 90 | 60 | 0 |
| **Standards compliance** | 5% | 50 | 70 | 100 | 50 |
| **Documentation** | 5% | 40 | 95 | 70 | 40 |
| **Total Score** | | **63.5** | **82.75** | **69.5** | **68.0** |

**Winner**: xtensor (82.75/100)

---

## Final Recommendation

### Primary Recommendation: Gradual xtensor Adoption

**Rationale**:
1. **Proven technology** - xtensor is mature, well-tested, and widely used
2. **Feature-rich** - Gains broadcasting, lazy eval, SIMD without writing code
3. **NumPy familiarity** - Easier for scientific computing users
4. **Incremental migration** - Can wrap with existing API
5. **Future-proof** - Active development ensures long-term viability

**Implementation Plan**:

**Quarter 1: Foundation**
- Add xtensor dependency to CMake
- Create TypedDataArray wrapper around xtensor
- Maintain 100% API compatibility
- Add integration tests

**Quarter 2: Feature Migration**
- Migrate slicing to use xtensor views
- Remove custom slicing implementation
- Benchmark performance improvements
- Update documentation

**Quarter 3: Feature Enhancement (Optional)**
- Expose broadcasting to advanced users
- Add lazy evaluation support
- Implement expression templates API
- Performance tuning

**Quarter 4: Stabilization**
- Deprecate old internal APIs
- Complete migration
- Performance benchmarks
- Update all documentation

### Alternative: Stay with Current + Enhancements

**If migration is not feasible**:
1. ✅ Keep comprehensive edge case tests (already added)
2. Add property-based testing
3. Implement SIMD for scale/offset operations
4. Add fuzzing for parser
5. Improve error messages
6. Consider xtensor for future features only

---

## Migration Code Examples

### Example 1: Wrapper Pattern (Minimal Disruption)

```cpp
// typed_data_array.hpp
#include <xtensor/xarray.hpp>
#include <xtensor/xview.hpp>

class TypedDataArray {
    // Internal storage now uses xtensor
    std::variant<
        xt::xarray<float>,
        xt::xarray<double>,
        xt::xarray<int>,
        xt::xarray<std::string>
    > m_array;
    
public:
    // Existing API - users see no change
    template<typename T>
    TypedDataArray slice(const std::string& slice_str) {
        auto& xarr = std::get<xt::xarray<T>>(m_array);
        
        // Parse "[2:8:2]" style string
        auto ranges = parse_slice_string(slice_str);
        
        // Apply using xtensor
        auto view = xt::strided_view(xarr, ranges);
        
        // Create new TypedDataArray with copy
        return TypedDataArray{xt::xarray<T>(view)};
    }
    
    // Helper to parse LibTokaMap slice string to xtensor format
    auto parse_slice_string(const std::string& str) {
        // "[2:8:2]" -> xt::range(2, 8, 2)
        // "[::-1]" -> xt::range(_, _, -1)
        // etc.
    }
};
```

### Example 2: Direct Integration (Clean Break)

```cpp
// New API design
namespace libtokamap::v2 {

template<typename T>
class Array {
    xt::xarray<T> m_array;
    
public:
    // Modern, clean API
    auto slice(auto... ranges) {
        return xt::view(m_array, ranges...);
    }
    
    // Support old string-based slicing
    auto slice(const std::string& slice_str) {
        return slice(parse_to_ranges(slice_str));
    }
    
    // Expose full xtensor power
    xt::xarray<T>& xtensor() { return m_array; }
};

} // namespace v2
```

---

## Conclusion

**xtensor** provides the best balance of features, maturity, and integration ease. While it adds a dependency, the benefits in reduced maintenance, extensive features, and performance optimizations outweigh the costs. The existing API can be preserved through a thin wrapper, making migration low-risk.

For projects requiring zero dependencies or C++23 alignment, **std::mdspan** is a viable lightweight alternative, though it requires keeping the custom slicing logic.

The **current implementation** should only be retained if:
- Migration resources are unavailable
- Dependency constraints are absolute
- Current functionality fully meets all needs

Even in the "keep current" scenario, the 78 new edge case tests provide essential quality assurance for the existing implementation.

---

## References

- [xtensor documentation](https://xtensor.readthedocs.io/)
- [xtensor GitHub](https://github.com/xtensor-stack/xtensor)
- [std::mdspan reference](https://en.cppreference.com/w/cpp/container/mdspan)
- [mdspan backport](https://github.com/kokkos/mdspan)
- [NumPy to xtensor cheatsheet](https://xtensor.readthedocs.io/en/latest/numpy.html)
- [Eigen documentation](https://eigen.tuxfamily.org/)
- [Beyond NumPy: xtensor article](https://www.einfochips.com/blog/beyond-numpy-exploring-xtensor-for-c-scientific-computing/)