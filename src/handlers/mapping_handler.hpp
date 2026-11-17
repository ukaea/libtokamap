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

    /**
     * List the experiments (machines) for which mappings have been found.
     *
     * @return sorted list of experiment names
     */
    [[nodiscard]] std::vector<std::string> list_experiments() const;

    /**
     * List the groups (e.g. "magnetics", "equilibrium") defined for an experiment.
     *
     * @param experiment experiment name (case-insensitive)
     * @return sorted list of group names
     */
    [[nodiscard]] std::vector<std::string> list_groups(const std::string& experiment) const;

    /**
     * List the mapping paths available for a group of an experiment.
     *
     * Reads the keys of the partition's mappings.json directly from disk, so no data sources or
     * custom function libraries need to be registered. The returned paths are prefixed with the
     * group name, matching the form accepted by map(), e.g. "magnetics/coil[#]/name". Index
     * placeholders ("#") are returned as written in the mapping files.
     *
     * @param experiment experiment name (case-insensitive)
     * @param group group name, e.g. "magnetics"
     * @param attributes partition attributes used to select the mapping directory, e.g. {"shot": 45000}
     * @param include_internal include internal helper mappings (keys starting with '_')
     * @return sorted list of mapping paths
     */
    [[nodiscard]] std::vector<std::string> list_mappings(const std::string& experiment, const std::string& group,
                                                         const nlohmann::json& attributes,
                                                         bool include_internal = false) const;

  private:
    [[nodiscard]] const ExperimentMappings& get_experiment(const std::string& experiment) const;

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
