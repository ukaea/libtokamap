# Contributing to LibTokaMap

Thank you for your interest in contributing to LibTokaMap! This guide will help you get started with contributing to the project.

## Table of Contents

- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Coding Standards](#coding-standards)
- [Testing Guidelines](#testing-guidelines)
- [Documentation](#documentation)
- [Submitting Changes](#submitting-changes)
- [Review Process](#review-process)
- [Code of Conduct](#code-of-conduct)
- [Getting Help](#getting-help)

## Getting Started

### Prerequisites

Before contributing, ensure you have:

- **C++20 compatible compiler**: GCC 10+, Clang 11+, or MSVC 2019+
- **CMake 3.15 or later**
- **Git** for version control
- **Familiarity with C++ and modern C++ practices**

### Setting Up Your Development Environment

1. **Fork the repository**
   ```bash
   # Visit https://github.com/ukaea/libtokamap and click "Fork"
   ```

2. **Clone your fork**
   ```bash
   git clone https://github.com/YOUR_USERNAME/libtokamap.git
   cd libtokamap
   ```

3. **Add upstream remote**
   ```bash
   git remote add upstream https://github.com/ukaea/libtokamap.git
   ```

4. **Install dependencies**
   ```bash
   # Dependencies: nlohmann/json, Pantor/Inja, ExprTk, GSL-lite
   # Install via your package manager or build from source
   ```

5. **Build the project**
   ```bash
   cmake -Bbuild -DENABLE_TESTING=ON -DENABLE_EXAMPLES=ON
   cmake --build build
   ```

6. **Run tests**
   ```bash
   cmake --build build --target test
   # or
   ctest --test-dir build --output-on-failure
   ```

## Development Workflow

### Branching Strategy

We use a feature branch workflow:

- **`main`**: Stable release branch
- **`develop`**: Integration branch for new features
- **`feature/*`**: Feature development branches
- **`bugfix/*`**: Bug fix branches
- **`hotfix/*`**: Critical fixes for production

### Creating a Feature Branch

1. **Update your local develop branch**
   ```bash
   git checkout develop
   git pull upstream develop
   ```

2. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make your changes**
   - Write code following our coding standards
   - Add tests for new functionality
   - Update documentation as needed

4. **Commit your changes**
   ```bash
   git add .
   git commit -m "feature: brief description"
   ```

### Commit Message Guidelines

We follow the commit format:

```
<type>: <summary>

<body>

<footer>
```

**Types:**

- `feature`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks
- `perf`: Performance improvements

**Example:**

```
feature: add HDF5 data source support

Implement HDF5DataSource class with factory pattern.
Includes comprehensive error handling and dataset slicing.

Closes #123
```

### Keeping Your Branch Up to Date

```bash
git fetch upstream
git rebase upstream/develop
```

If conflicts occur, resolve them and continue:

```bash
# Fix conflicts in your editor
git add <resolved-files>
git rebase --continue
```

## Coding Standards

### C++ Style Guide

LibTokaMap follows a consistent coding style based on LLVM conventions:

#### Formatting

We use `clang-format` for automatic formatting. The configuration is in `.clang-format`:

```bash
# Format your code before committing
clang-format -i src/**/*.cpp src/**/*.hpp
```

Key formatting rules:

- **Indentation**: 4 spaces (no tabs)
- **Column limit**: 120 characters
- **Brace style**: Linux style (opening brace on same line for functions)
- **Pointer alignment**: Left (`Type* ptr` not `Type *ptr`)

#### Naming Conventions

- **Classes/Structs**: `PascalCase`
  ```cpp
  class MappingHandler { };
  struct DataSourceArgs { };
  ```

- **Functions/Methods**: `snake_case`
  ```cpp
  void process_mapping();
  TypedDataArray get_data();
  ```

- **Variables**: `snake_case`
  ```cpp
  int data_size;
  std::string file_path;
  ```

- **Member Variables**: `m_` prefix with `snake_case`
  ```cpp
  class Example {
  private:
      int m_value;
      std::string m_name;
  };
  ```

- **Constants**: `PascalCase`
  ```cpp
  constexpr int MaxBufferSize = 1024;
  ```

- **Namespaces**: `snake_case`
  ```cpp
  namespace libtokamap { }
  ```

#### Modern C++ Practices

- **Use C++20 features** where appropriate:
  - `std::span` for array views
  - Concepts for template constraints
  - Ranges for algorithms
  - `std::format` for string formatting (when available)

- **Prefer RAII** for resource management
  ```cpp
  // Good
  auto resource = std::make_unique<Resource>();
  
  // Avoid
  Resource* resource = new Resource();
  // ... manual cleanup
  ```

- **Use `const` and `constexpr`** liberally
  ```cpp
  constexpr size_t calculate_size() { return 42; }
  void process(const std::string& input);
  ```

- **Prefer `auto`** for complex types
  ```cpp
  auto result = mapping_handler.map(experiment, path, type, rank, attrs);
  ```

- **Use smart pointers** over raw pointers
  ```cpp
  std::unique_ptr<DataSource> source;
  std::shared_ptr<RamCache> cache;
  ```

#### Error Handling

- **Use exceptions** for exceptional conditions
  ```cpp
  if (!file_exists(path)) {
      throw libtokamap::DataSourceError("File not found: " + path);
  }
  ```

- **Document exceptions** in function comments
  ```cpp
  /**
   * @throws DataSourceError if the file cannot be read
   * @throws ValidationError if the data format is invalid
   */
  TypedDataArray load_data(const std::string& path);
  ```

- **Catch specific exceptions** rather than catching all
  ```cpp
  try {
      process_data();
  } catch (const libtokamap::DataSourceError& e) {
      handle_data_error(e);
  } catch (const libtokamap::ValidationError& e) {
      handle_validation_error(e);
  }
  ```

### Code Documentation

Use Doxygen-style comments for public APIs:

```cpp
/**
 * @brief Maps experiment data to a typed array
 * 
 * @param experiment The experiment name
 * @param path The data path within the experiment
 * @param data_type The expected data type as std::type_index
 * @param rank The expected array rank (dimensionality)
 * @param extra_attributes Additional attributes for mapping resolution
 * 
 * @return TypedDataArray containing the mapped data
 * 
 * @throws MappingError if the mapping cannot be resolved
 * @throws DataSourceError if data retrieval fails
 * 
 * @example
 * ```cpp
 * auto result = handler.map("ITER_001", "magnetics/flux", 
 *                          std::type_index{typeid(double)}, 2, {});
 * ```
 */
TypedDataArray map(const std::string& experiment,
                   const std::string& path,
                   std::type_index data_type,
                   int rank,
                   const nlohmann::json& extra_attributes);
```

## Testing Guidelines

### Test Requirements

- **All new features** must include tests
- **Bug fixes** should include regression tests
- **Tests must pass** before submitting a pull request

### Writing Tests

We use Catch2 for testing:

```cpp
#include <catch2/catch_test_macros.hpp>
#include "your_component.hpp"

TEST_CASE("Feature works correctly", "[feature]") {
    SECTION("Basic functionality") {
        YourComponent component;
        auto result = component.process();
        
        REQUIRE(result.is_valid());
        REQUIRE(result.size() == expected_size);
    }
    
    SECTION("Error handling") {
        YourComponent component;
        REQUIRE_THROWS_AS(component.invalid_operation(), 
                         libtokamap::ProcessingError);
    }
}
```

### Test Categories

Use tags to categorize tests:

- `[unit]`: Unit tests for individual components
- `[integration]`: Integration tests for component interaction
- `[performance]`: Performance benchmarks
- `[subset]`: Subset operation tests
- `[config]`: Configuration tests

Also tag the tests with the code unit being tested, i.e. `[indices]` when testing the indices component.

### Running Tests

```bash
# Run all tests
ctest

# Run specific test category
ctest -R subset

# Run with verbose output
ctest --output-on-failure

# Run tests in parallel
ctest -j8
```

### Test Coverage

We aim for high test coverage:

```bash
# Build with coverage enabled
cmake .. -DENABLE_TESTING=ON -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build
cmake --build build --target test
cmake --build build --target coverage
```

Note: currently, this only works with LLVM, and requires the `llvm-cov` tool installed.

## Documentation

### Documentation Requirements

- **Public APIs** must be documented with Doxygen comments
- **Complex algorithms** should have explanatory comments
- **Architecture changes** should update the knowledge graph
- **New features** should update the README and user guide

## Submitting Changes

### Pre-Submission Checklist

Before submitting a pull request:

- [ ] Code follows the style guide
- [ ] All tests pass
- [ ] New tests added for new functionality
- [ ] Documentation updated
- [ ] Commits are well-formatted
- [ ] Branch is rebased on latest develop
- [ ] No compiler warnings

### Creating a Pull Request

1. **Push your branch**
   ```bash
   git push origin feature/your-feature-name
   ```

2. **Open a pull request**
   - Go to the GitHub repository
   - Click "New Pull Request"
   - Select `develop` as the base branch
   - Select your feature branch as the compare branch

3. **Fill out the PR template**
   ```markdown
   ## Description
   Brief description of your changes
   
   ## Related Issues
   Closes #123
   
   ## Changes Made
   - Added feature X
   - Fixed bug Y
   - Updated documentation
   
   ## Screenshots (if applicable)
   
   ## Checklist
   - [ ] Code follows style guide
   - [ ] Documentation updated
   - [ ] Tests added/updated
   - [ ] No breaking changes (or documented)
   ```

### Pull Request Guidelines

- **Keep PRs focused**: One feature or fix per PR
- **Keep PRs small**: Easier to review and merge
- **Write clear descriptions**: Explain what and why
- **Link to issues**: Reference related issues
- **Be responsive**: Address review feedback promptly

## Review Process

### What to Expect

1. **Automated checks**: CI will run tests and linting
2. **Code review**: Maintainers will review your code
3. **Feedback**: You may receive requests for changes
4. **Approval**: Once approved, your PR will be merged

### Addressing Review Comments

```bash
# Make requested changes
git add .
git commit -m "Address review comments"
git push origin feature/your-feature-name
```

## Types of Contributions

### Bug Reports

Report bugs via GitHub Issues:

**Template:**
```markdown
**Description**
Clear description of the bug

**To Reproduce**
Steps to reproduce the behavior

**Expected Behavior**
What you expected to happen

**Environment**
- OS: [e.g., Ubuntu 22.04]
- Compiler: [e.g., GCC 11.2]
- LibTokaMap version: [e.g., 0.1.0]

**Additional Context**
Any other relevant information
```

### Feature Requests

Suggest features via GitHub Issues:

**Template:**
```markdown
**Feature Description**
Clear description of the proposed feature

**Use Case**
Why is this feature needed?

**Proposed Implementation**
How might this be implemented?

**Alternatives Considered**
Other approaches you've considered
```

### Documentation Improvements

Documentation improvements are always welcome:
- Fix typos or unclear explanations
- Add examples
- Improve API documentation
- Update architecture documentation

### Code Contributions

Areas where contributions are especially welcome:

- **Data Source Implementations**: New data source plugins
- **Performance Optimizations**: SIMD, caching, algorithms
- **Testing**: Improve test coverage
- **Examples**: More usage examples
- **Platform Support**: Windows, macOS compatibility

## Getting Help

### Resources

- **Documentation**: Check the [developer resources](developer_resources/index.md)
- **Examples**: See the `examples/` directory
- **Issues**: Search existing GitHub issues
- **Discussions**: GitHub Discussions for questions

### Communication Channels

- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: General questions and discussions
- **Pull Requests**: Code reviews and technical discussions

### Asking Questions

When asking for help:

1. **Search first**: Check if your question has been answered
2. **Be specific**: Provide context and details
3. **Include examples**: Code snippets, error messages
4. **Be patient**: Maintainers contribute in their spare time

## Development Guidelines

### Performance Considerations

- **Profile before optimizing**: Use profiling tools
- **Benchmark changes**: Measure performance impact
- **Consider memory usage**: Monitor allocations
- **Use appropriate data structures**: Choose wisely

### Security Considerations

- **Validate inputs**: Check user-provided data
- **Avoid buffer overflows**: Use safe string operations
- **Handle errors securely**: Don't leak sensitive information
- **Follow best practices**: OWASP, CWE guidelines

### Backward Compatibility

- **Maintain API stability**: Avoid breaking changes
- **Deprecate before removing**: Give users time to migrate
- **Version appropriately**: Follow semantic versioning
- **Document breaking changes**: Clear migration guide

## Recognition

Contributors are recognized in:
- Git commit history
- Release notes
- Project README (for significant contributions)

Thank you for contributing to LibTokaMap! Your efforts help make this project better for everyone.

## Code of Conduct

We are committed to providing a welcoming and inclusive environment for all contributors. Please be respectful and professional in all interactions.

### Our Standards

- **Be respectful**: Treat everyone with respect and consideration
- **Be collaborative**: Work together constructively and openly
- **Be inclusive**: Welcome diverse perspectives and experiences
- **Be professional**: Maintain a professional demeanor in all communications
- 
## License

By contributing to LibTokaMap, you agree that your contributions will be licensed under the same license as the project. See the LICENSE file for details.

---

*For questions about contributing, please open a GitHub Discussion or contact the maintainers.*
