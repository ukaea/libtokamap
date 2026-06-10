#pragma once

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/data_source_cache.hpp"
#include "utils/ram_cache.hpp"
#include "utils/typed_data_array.hpp"

#include <cstddef>
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace libtokamap
{

using DataSourceName = std::string;
using DataSourceArgs = std::unordered_map<std::string, nlohmann::json>;

class DataSource
{
  public:
    DataSource() = default;
    virtual TypedDataArray get(const DataSourceArgs& map_args, const MapArguments& arguments, RamCache* ram_cache) = 0;
    virtual ~DataSource() = default;

    DataSource(DataSource&& other) = default;
    DataSource(const DataSource& other) = default;
    DataSource& operator=(DataSource&& other) = default;
    DataSource& operator=(const DataSource& other) = default;
};

class DataSourceMapping : public Mapping
{
  public:
    DataSourceMapping() = delete;
    DataSourceMapping(std::string name, DataSource* m_data_source, DataSourceArgs data_source_args,
                      std::optional<float> offset, std::optional<float> scale, std::optional<std::string> slice);

    [[nodiscard]] TypedDataArray map(const MapArguments& arguments) const override;

  private:
    DataSourceName m_name;
    DataSource* m_data_source;
    DataSourceArgs m_data_source_args;
    std::optional<float> m_offset;
    std::optional<float> m_scale;
    std::optional<std::string> m_slice;
};

} // namespace libtokamap
