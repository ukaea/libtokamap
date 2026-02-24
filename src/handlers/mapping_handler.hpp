#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "map_types/data_source_mapping.hpp"
#include "utils/library_loader.hpp"
#include "utils/ram_cache.hpp"
#include "utils/typed_data_array.hpp"
#include "utils/types.hpp"
#include "valijson_nlohmann_bundled.hpp"

struct PluginList;

namespace libtokamap
{

class DataSource;

using DataSourceRegistry = std::unordered_map<std::string, std::unique_ptr<libtokamap::DataSource>>;
using DataSourceFactoryRegistry = std::unordered_map<std::string, DataSourceFactory>;

class MappingCounts
{
  public:
    MappingCounts() = default;
    void increment(const MappingName& name) { m_counts[name]++; }
    [[nodiscard]] int get(const MappingName& name) const { return m_counts.contains(name) ? m_counts.at(name) : 0; };

  private:
    std::unordered_map<MappingName, int> m_counts;
};

class MappingHandler
{
  public:
    MappingHandler() = default;

    void reset();
    void init(const std::filesystem::path& config_path);
    void init(const nlohmann::json& config);

    [[nodiscard]] TypedDataArray map(const std::string& experiment, const std::string& path, DataType data_type,
                                     int rank, const nlohmann::json& extra_attributes);

    void register_data_source_factory(const std::string& factory_name, const std::filesystem::path& library_path);
    void register_data_source_factory(const std::string& factory_name, DataSourceFactory factory);

    void register_data_source(const std::string& name, std::unique_ptr<libtokamap::DataSource> data_source);
    void register_data_source(const std::string& name, const std::string& factory_name,
                              const DataSourceFactoryArgs& args);
    void unregister_data_source(const std::string& name);

    void register_custom_function(LibraryFunction custom_function);
    void unregister_custom_function(const std::string& library_name, const std::string& function_name);

    void load_custom_function_library(const std::filesystem::path& library_path);

  private:
    void load_experiment(const ExperimentName& experiment, const nlohmann::json& attributes);
    libtokamap::MappingStore init_mappings(const nlohmann::json& data, const nlohmann::json& group_attributes);

    void load_data_source_factory(const std::string& name, const std::string& library);
    void load_data_source(const std::string& name, const nlohmann::json& args);

    DataSourceFactoryRegistry m_data_source_factories;
    DataSourceRegistry m_data_sources;
    ExperimentRegisterStore m_experiment_register;
    bool m_init = false;
    bool m_trace_enabled = false;

    std::string m_dd_version;
    std::filesystem::path m_mapping_dir;
    std::shared_ptr<libtokamap::RamCache> m_ram_cache;
    bool m_cache_enabled = false;
    valijson::Schema m_mappings_schema;
    valijson::Schema m_globals_schema;
    valijson::Schema m_mapping_config_schema;
    std::vector<libtokamap::LibraryFunction> m_library_functions;
    MappingCounts m_mapping_counts;

    constexpr static int MappingCacheThreshold = 2;
    std::unordered_map<std::string, TypedDataArray> m_mapping_cache;
};

} // namespace libtokamap
