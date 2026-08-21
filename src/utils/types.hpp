#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "map_types/base_mapping.hpp"

namespace libtokamap
{

using MappingName = std::string;
using ExperimentName = std::string;
using GroupName = std::string;

using GroupMappingPath = std::unordered_map<GroupName, std::filesystem::path>;

using MappingStore = std::unordered_map<MappingName, std::unique_ptr<Mapping>>;
struct MappingPair {
    nlohmann::json globals;
    MappingStore mappings;

    MappingPair() = default;
    MappingPair(const MappingPair& other) = delete;
    MappingPair& operator=(const MappingPair& other) = delete;
    MappingPair(MappingPair&& other) noexcept
    {
        std::swap(globals, other.globals);
        std::swap(mappings, other.mappings);
    }
    MappingPair& operator=(MappingPair&& other) noexcept
    {
        std::swap(globals, other.globals);
        std::swap(mappings, other.mappings);
        return *this;
    }
    ~MappingPair() = default;
};
using PartitionMappings = std::unordered_map<nlohmann::json, MappingPair>;
using GroupMappings = std::unordered_map<GroupName, PartitionMappings>;

enum class DirectorySelector : uint8_t { Undefined, MaxBelow, MinAbove, Exact, Closest };

NLOHMANN_JSON_SERIALIZE_ENUM(libtokamap::DirectorySelector, {
                                                                {libtokamap::DirectorySelector::Undefined, nullptr},
                                                                {libtokamap::DirectorySelector::MaxBelow, "MAX_BELOW"},
                                                                {libtokamap::DirectorySelector::MinAbove, "MIN_ABOVE"},
                                                                {libtokamap::DirectorySelector::Exact, "EXACT"},
                                                                {libtokamap::DirectorySelector::Closest, "CLOSEST"},
                                                            })

struct MappingPartition {
    std::string attribute;
    DirectorySelector selector;
};

inline void from_json(const nlohmann::json& json, libtokamap::MappingPartition& mapping_partition)
{
    json.at("attribute").get_to(mapping_partition.attribute);
    json.at("selector").get_to(mapping_partition.selector);
}

using PartitionList = std::vector<MappingPartition>;

struct ExperimentMappings {
    bool is_loaded = false;
    PartitionList partition_list;
    std::vector<GroupName> groups;
    GroupMappings group_mappings;
    nlohmann::json top_level_globals;
    std::filesystem::path root_path;

    ExperimentMappings() = default;
    ExperimentMappings(PartitionList partition_list, std::vector<GroupName> groups, std::filesystem::path root_path)
        : partition_list(std::move(partition_list)), groups(std::move(groups)), root_path(std::move(root_path))
    {
    }
    ExperimentMappings(const ExperimentMappings& other) = delete;
    ExperimentMappings& operator=(const ExperimentMappings& other) = delete;
    ExperimentMappings(ExperimentMappings&& other) noexcept
    {
        std::swap(is_loaded, other.is_loaded);
        std::swap(partition_list, other.partition_list);
        std::swap(groups, other.groups);
        std::swap(group_mappings, other.group_mappings);
        std::swap(top_level_globals, other.top_level_globals);
        std::swap(root_path, other.root_path);
    }
    ExperimentMappings& operator=(ExperimentMappings&& other) noexcept
    {
        std::swap(is_loaded, other.is_loaded);
        std::swap(partition_list, other.partition_list);
        std::swap(groups, other.groups);
        std::swap(group_mappings, other.group_mappings);
        std::swap(top_level_globals, other.top_level_globals);
        std::swap(root_path, other.root_path);
        return *this;
    }
    ~ExperimentMappings() = default;
};

using ExperimentRegisterStore = std::unordered_map<ExperimentName, ExperimentMappings>;

} // namespace libtokamap
