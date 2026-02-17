#pragma once

#include <any>
#include <filesystem>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "utils/typed_data_array.hpp"
#include "utils/os_utils.hpp"

namespace libtokamap
{

using CustomMappingInputs = std::unordered_map<std::string, libtokamap::TypedDataArray>;
using CustomMappingParams = nlohmann::json;

using LibraryName = std::string;
using FunctionName = std::string;

class LibraryFunctionWrapper
{
  public:
    LibraryFunctionWrapper() = default;
    virtual libtokamap::TypedDataArray operator()(CustomMappingInputs& inputs,
                                                  const CustomMappingParams& params) const = 0;
    virtual ~LibraryFunctionWrapper() = default;

    LibraryFunctionWrapper(const LibraryFunctionWrapper& other) = delete;
    LibraryFunctionWrapper(LibraryFunctionWrapper&& other) = default;
    LibraryFunctionWrapper& operator=(LibraryFunctionWrapper&& other) = default;
    LibraryFunctionWrapper& operator=(const LibraryFunctionWrapper& other) = delete;
};

class LibraryFunctionPointer : public LibraryFunctionWrapper
{
  public:
    using FunctionType = std::function<TypedDataArray(CustomMappingInputs&, const CustomMappingParams&)>;
    explicit LibraryFunctionPointer(FunctionType function) : m_function(std::move(function)) {}
    libtokamap::TypedDataArray operator()(CustomMappingInputs& inputs, const CustomMappingParams& params) const override
    {
        return m_function(inputs, params);
    }

  private:
    FunctionType m_function;
};

class LibraryFunction
{
  public:
    LibraryFunction(LibraryName library_name, FunctionName function_name,
                    std::unique_ptr<LibraryFunctionWrapper> function)
        : m_library_name(std::move(library_name)), m_function_name(std::move(function_name)),
          m_function(std::move(function))
    {
        if (m_function == nullptr) {
            throw TokaMapError("Library function is null");
        }
    }
    ~LibraryFunction() = default;
    LibraryFunction(const LibraryFunction& other) = delete;
    LibraryFunction(LibraryFunction&& other) = default;
    LibraryFunction& operator=(LibraryFunction&& other) = default;
    LibraryFunction& operator=(const LibraryFunction& other) = delete;

    [[nodiscard]] bool matches(const LibraryName& library_name, const FunctionName& function_name) const
    {
        return m_library_name == library_name && m_function_name == function_name;
    }

    [[nodiscard]] libtokamap::TypedDataArray call(CustomMappingInputs& inputs, const CustomMappingParams& params) const
    {
        return (*m_function)(inputs, params);
    }

  private:
    LibraryName m_library_name;
    FunctionName m_function_name;
    std::unique_ptr<LibraryFunctionWrapper> m_function;
};

struct LibraryEntryInterface {
    std::vector<LibraryFunction> functions;
};

std::vector<libtokamap::LibraryFunction> load_custom_functions(const std::filesystem::path& custom_function_library);

class DataSource;

using DataSourceFactoryArgs = std::unordered_map<std::string, std::any>;
using DataSourceFactory = std::function<std::unique_ptr<DataSource>(const DataSourceFactoryArgs&)>;

struct FactoryEntryInterface {
    DataSourceFactory function;
};

DataSourceFactory load_data_source_factory(const std::filesystem::path& library_path);

template <typename T> std::string to_string()
{
    return demangle(typeid(T).name());
}

template <typename T> auto get_arg(const DataSourceFactoryArgs& args, const char* variable) -> T
{
    if (!args.contains(variable)) {
        throw libtokamap::TokaMapError("Missing factory argument '" + std::string(variable) + "'");
    }
    try {
        return std::any_cast<T>(args.at(variable));
    } catch (const std::bad_any_cast&) {
        throw libtokamap::ConfigurationError("Expected type '" + to_string<T>() + "' for variable '" + variable + "'");
    }
}

} // namespace libtokamap
