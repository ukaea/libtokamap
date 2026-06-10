#include "ram_cache.hpp"

#include <string>

bool libtokamap::RamCache::contains(const std::string& key) const
{
    const std::lock_guard lock{m_mutex};
    return m_entries.contains(key);
}
