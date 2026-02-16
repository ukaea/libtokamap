#include "mapping_locator.hpp"

#include <cstdlib>
#include <filesystem>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "utils/types.hpp"

namespace
{
std::string select_exact(const std::vector<std::string>& subdirectories, const std::string& name)
{
    for (const auto& subdir : subdirectories) {
        if (subdir == name) {
            return subdir;
        }
    }
    throw libtokamap::ParameterError{"No exact match found for " + name};
}

std::string select_closest(const std::vector<std::string>& subdirectories, int value)
{
    int closest_distance = std::numeric_limits<int>::max();
    std::string closest_subdir;
    for (const auto& subdir : subdirectories) {
        int dir_value = std::stoi(subdir);
        int distance = std::abs(value - dir_value);
        if (distance < closest_distance) {
            closest_distance = distance;
            closest_subdir = subdir;
        }
    }
    return closest_subdir;
}

std::string select_max_below(const std::vector<std::string>& subdirectories, int value)
{
    int max_below = std::numeric_limits<int>::min();
    std::string max_below_subdir;
    for (const auto& subdir : subdirectories) {
        int dir_value = std::stoi(subdir);
        if (dir_value < value && dir_value > max_below) {
            max_below = dir_value;
            max_below_subdir = subdir;
        }
    }
    if (max_below == std::numeric_limits<int>::min()) {
        throw libtokamap::ParameterError{"No subdirectories found below the given value"};
    }
    return max_below_subdir;
}

std::string select_min_above(const std::vector<std::string>& subdirectories, int value)
{
    int min_above = std::numeric_limits<int>::max();
    std::string min_above_subdir;
    for (const auto& subdir : subdirectories) {
        int dir_value = std::stoi(subdir);
        if (dir_value > value && dir_value < min_above) {
            min_above = dir_value;
            min_above_subdir = subdir;
        }
    }
    if (min_above == std::numeric_limits<int>::max()) {
        throw libtokamap::ParameterError{"No subdirectories found above the given value"};
    }
    return min_above_subdir;
}

template <typename T>
std::filesystem::path select_subdirectory(const std::filesystem::path& directory,
                                          libtokamap::DirectorySelector selector, const T& value)
{
    std::vector<std::string> subdirectories;
    for (const auto& dir : std::filesystem::directory_iterator{directory}) {
        if (std::filesystem::is_directory(dir)) {
            subdirectories.push_back(dir.path().filename().string());
        }
    }
    return directory / libtokamap::detail::select_subdirectory(subdirectories, selector, value);
}
} // namespace

std::string libtokamap::detail::select_subdirectory(const std::vector<std::string>& subdirectories,
                                                    DirectorySelector selector, int value)
{
    switch (selector) {
        case DirectorySelector::Closest:
            return select_closest(subdirectories, value);
        case DirectorySelector::Exact:
            return select_exact(subdirectories, std::to_string(value));
        case DirectorySelector::MaxBelow:
            return select_max_below(subdirectories, value);
        case DirectorySelector::MinAbove:
            return select_min_above(subdirectories, value);
        default:
            throw ParameterError{"Invalid selector for integer attribute"};
    }
}

std::string libtokamap::detail::select_subdirectory(const std::vector<std::string>& subdirectories,
                                                    DirectorySelector selector, const std::string& value)
{
    switch (selector) {
        case DirectorySelector::Exact:
            return select_exact(subdirectories, value);
        default:
            throw ParameterError{"Invalid selector for string attribute"};
    }
}

std::filesystem::path libtokamap::find_partition_directory(std::filesystem::path mapping_dir,
                                                           const PartitionList& partitions,
                                                           const nlohmann::json& attributes)
{
    if (!std::filesystem::is_directory(mapping_dir)) {
        throw ConfigurationError{mapping_dir.string() + " is not a directory"};
    }

    for (const auto& [name, selector] : partitions) {
        if (!attributes.contains(name)) {
            throw ParameterError{"required attribute '" + name + "' not provided"};
        }
        const auto& attribute = attributes.at(name);
        if (attribute.is_number_integer()) {
            const auto value = attribute.get<int>();
            mapping_dir = select_subdirectory(mapping_dir, selector, value);
        } else if (attribute.is_string()) {
            const auto& value = attribute.get<std::string>();
            mapping_dir = select_subdirectory(mapping_dir, selector, value);
        } else {
            throw ParameterError{"attribute '" + name + "' must be a integer or string"};
        }
    }

    return mapping_dir;
}

nlohmann::json libtokamap::find_partition_attributes(const PartitionList& partitions, const nlohmann::json& attributes)
{
    nlohmann::json result = {};
    for (const auto& [name, _] : partitions) {
        if (!attributes.contains(name)) {
            throw ParameterError{"required attribute '" + name + "' not provided"};
        }
        result[name] = attributes.at(name);
    }
    return result;
}
