#pragma once

#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <optional>
#include <utility>

/*
 *
 * NOTES:
 *
 * using char instead of byte to avoid extra casting from/to datablock
 *
 */

namespace libtokamap
{

class CacheEntry {
public:
    CacheEntry() = default;
    virtual ~CacheEntry() = default;
    CacheEntry(CacheEntry&& other) = default;
    CacheEntry(const CacheEntry& other) = delete;
    CacheEntry& operator=(CacheEntry&& other) = default;
    CacheEntry& operator=(const CacheEntry& other) = delete;

    [[nodiscard]] virtual size_t size() const = 0;
};

constexpr int default_size = 100;

class RamCache
{
  public:
    RamCache()
    {
        m_entries.reserve(m_max_size);
    }

    explicit RamCache(uint32_t max_size) : m_max_size{max_size}
    {
        m_entries.reserve(m_max_size);
    }

    void add(std::string key, std::unique_ptr<CacheEntry> entry)
    {
        const std::lock_guard lock{m_mutex};
        if (m_max_size == 0) {
            return;
        }

        if (m_entries.contains(key)) {
            m_entries[key] = std::move(entry);
            touch(key);
            return;
        }

        if (m_entries.size() >= m_max_size) {
            drop_entries();
        }
        m_lru_order.push_front(key);
        m_lru_positions[key] = m_lru_order.begin();
        m_entries[key] = std::move(entry);
    }

    [[nodiscard]] bool contains(const std::string& key) const;

    [[nodiscard]] std::optional<CacheEntry*> get(const std::string& key) {
        const std::lock_guard lock{m_mutex};
        if (!m_entries.contains(key)) {
            return {};
        }
        touch(key);
        return m_entries.at(key).get();
    }

  private:
    const uint32_t m_max_size = default_size;
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::unique_ptr<CacheEntry>> m_entries;
    std::list<std::string> m_lru_order;
    std::unordered_map<std::string, std::list<std::string>::iterator> m_lru_positions;

    void touch(const std::string& key)
    {
        auto position = m_lru_positions.at(key);
        m_lru_order.splice(m_lru_order.begin(), m_lru_order, position);
        m_lru_positions[key] = m_lru_order.begin();
    }

    void drop_entries() {
        if (m_lru_order.empty()) {
            return;
        }
        const std::string key = m_lru_order.back();
        m_lru_order.pop_back();
        m_lru_positions.erase(key);
        m_entries.erase(key);
    }
};

} // namespace ram_cache
