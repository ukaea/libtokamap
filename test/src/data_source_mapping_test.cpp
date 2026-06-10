#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "map_types/data_source_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/ram_cache.hpp"

using namespace libtokamap;

class TestDataSource : public libtokamap::DataSource
{
    TypedDataArray get(const DataSourceArgs& /*map_args*/, const MapArguments& /*arguments*/,
                       libtokamap::RamCache* /*ram_cache*/) override
    {
        return {};
    }
};

class TestCacheEntry : public libtokamap::CacheEntry
{
  public:
    explicit TestCacheEntry(std::string value) : m_value{std::move(value)} {}

    [[nodiscard]] size_t size() const override { return m_value.size(); }
    [[nodiscard]] std::string_view value() const { return m_value; }

  private:
    std::string m_value;
};

TEST_CASE("Creating new PluginMapping", "[plugin_mapping]")
{
    SECTION("Create mock data source")
    {
        auto test_source = std::make_unique<TestDataSource>();

        DataSourceArgs request_args = {};
        std::optional<float> offset = {};
        std::optional<float> scale = {};
        std::optional<std::string> slice = {};
        std::optional<std::string> function = {};
        auto mapping =
            std::make_unique<DataSourceMapping>("test", test_source.get(), request_args, offset, scale, slice);
        REQUIRE(mapping != nullptr);
    }
}

TEST_CASE("RamCache evicts least recently used entries", "[ram_cache]")
{
    RamCache cache{2};

    cache.add("first", std::make_unique<TestCacheEntry>("one"));
    cache.add("second", std::make_unique<TestCacheEntry>("two"));
    REQUIRE(cache.get("first").has_value());

    cache.add("third", std::make_unique<TestCacheEntry>("three"));

    REQUIRE(cache.contains("first"));
    REQUIRE_FALSE(cache.contains("second"));
    REQUIRE(cache.contains("third"));
}

TEST_CASE("RamCache overwrites existing keys", "[ram_cache]")
{
    RamCache cache{2};

    cache.add("key", std::make_unique<TestCacheEntry>("old"));
    cache.add("key", std::make_unique<TestCacheEntry>("new"));

    auto entry = cache.get("key");
    REQUIRE(entry.has_value());
    const auto* typed_entry = dynamic_cast<TestCacheEntry*>(entry.value());
    REQUIRE(typed_entry != nullptr);
    REQUIRE(typed_entry->value() == "new");
}
