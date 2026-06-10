#pragma once

#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>

#include "utils/typed_data_array.hpp"

namespace libtokamap
{

struct DataSourceCacheKey {
    const void* data_source = nullptr;
    std::string name;
    nlohmann::json args;

    bool operator==(const DataSourceCacheKey& other) const
    {
        return data_source == other.data_source && name == other.name && args == other.args;
    }
};

struct DataSourceCacheKeyHash {
    size_t operator()(const DataSourceCacheKey& key) const
    {
        size_t hash = 0;
        constexpr std::size_t offset = 0x9e3779b9;
        auto hash_combine = [&](std::size_t value) {
            constexpr std::size_t left_shift = 6;
            constexpr std::size_t right_shift = 2;
            hash ^= value + offset + (hash << left_shift) + (hash >> right_shift);
        };

        hash_combine(std::hash<const void*>{}(key.data_source));
        hash_combine(std::hash<std::string>{}(key.name));
        hash_combine(std::hash<nlohmann::json>{}(key.args));
        return hash;
    }
};

class DataSourceCache
{
  public:
    [[nodiscard]] std::optional<TypedDataArray> get(const DataSourceCacheKey& key) const
    {
        const std::lock_guard lock{m_mutex};
        if (!m_entries.contains(key)) {
            return {};
        }
        return m_entries.at(key).clone();
    }

    void put(const DataSourceCacheKey& key, const TypedDataArray& value)
    {
        const std::lock_guard lock{m_mutex};
        m_entries[key] = value.clone();
    }

    void clear()
    {
        const std::lock_guard lock{m_mutex};
        m_entries.clear();
    }

  private:
    mutable std::mutex m_mutex;
    std::unordered_map<DataSourceCacheKey, TypedDataArray, DataSourceCacheKeyHash> m_entries;
};

} // namespace libtokamap
