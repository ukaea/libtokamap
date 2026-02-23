#include <cstddef>
#include <nlohmann/json.hpp>
#include <string>
#include <ostream>

#include "exceptions/exceptions.hpp"
#include "map_types/dim_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "map_types/value_mapping.hpp"
#include "utils/os_utils.hpp"
#include "utils/typed_data_array.hpp"

namespace
{
// define this _before_ including catch headers
std::ostream& operator<<(std::ostream& out, const libtokamap::DataType& value)
{
    out << libtokamap::data_type_name(value);
    return out;
}
} // namespace

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

namespace Catch
{
template <> struct StringMaker<libtokamap::DataType> {
    static std::string convert(const libtokamap::DataType& value)
    {
        return libtokamap::data_type_name(value);
    }
};
} // namespace Catch

using namespace libtokamap;

namespace
{

libtokamap::MapArguments
make_map_arguments(std::unordered_map<std::string, std::unique_ptr<libtokamap::Mapping>>& entries,
                   const libtokamap::DataType data_type, const int rank)
{
    static nlohmann::json empty_global_data = nlohmann::json::object();

    constexpr bool trace_enabled = true;
    constexpr bool cache_enabled = false;
    constexpr RamCache* ram_cache = nullptr;

    return MapArguments(entries, empty_global_data, data_type, rank, trace_enabled, cache_enabled, ram_cache);
}

} // namespace

TEST_CASE("DimMapping can be constructed from JSON", "[dim_mapping]")
{
    // Setup test fixture
    nlohmann::json test_json = {{"MAP_TYPE", "DIMENSION"}, {"DIM_PROBE", "test_dim/mapping/signal"}};

    SECTION("Constructor works with string probe name")
    {
        auto mapping = std::make_unique<DimMapping>(test_json.at("DIM_PROBE"));
        REQUIRE(mapping != nullptr);
    }

    // SECTION("Constructor works with different probe names")
    // {
    //     auto mapping1 = std::make_unique<DimMapping>("probe1");
    //     auto mapping2 = std::make_unique<DimMapping>("another_probe_name");
    //     REQUIRE(mapping1 != nullptr);
    //     REQUIRE(mapping2 != nullptr);
    // }
}

TEST_CASE("DimMapping returns expected dimension sizes", "[dim_mapping]")
{
    SECTION("Returns correct size for 1D int array")
    {
        auto dim_mapping = std::make_unique<DimMapping>("test_signal");

        nlohmann::json value_json = {1, 2, 3, 4, 5};
        auto value_mapping = std::make_unique<ValueMapping>(value_json);

        std::unordered_map<std::string, std::unique_ptr<Mapping>> entries;
        entries["test_signal"] = std::move(value_mapping);
        MapArguments map_args = make_map_arguments(entries, DataType::UInt64, 1);

        auto dim_return = dim_mapping->map(map_args);

        REQUIRE(!dim_return.empty());
        REQUIRE(dim_return.data_type() == DataType::UInt64);
        REQUIRE(dim_return.rank() == 0);
        REQUIRE(*reinterpret_cast<const int*>(dim_return.buffer()) == 5);
    }

    SECTION("Returns correct size for larger 1D array")
    {
        auto dim_mapping = std::make_unique<DimMapping>("test_array");

        std::vector<int> test_data(100);
        std::iota(test_data.begin(), test_data.end(), 1);
        nlohmann::json value_json = test_data;
        auto value_mapping = std::make_unique<ValueMapping>(value_json);

        std::unordered_map<std::string, std::unique_ptr<Mapping>> entries;
        entries["test_array"] = std::move(value_mapping);
        MapArguments map_args = make_map_arguments(entries, DataType::UInt64, 1);

        auto dim_return = dim_mapping->map(map_args);

        REQUIRE(!dim_return.empty());
        REQUIRE(dim_return.data_type() == DataType::UInt64);
        REQUIRE(dim_return.rank() == 0);
        REQUIRE(*reinterpret_cast<const int*>(dim_return.buffer()) == 100);
    }

    SECTION("Works with float arrays")
    {
        auto dim_mapping = std::make_unique<DimMapping>("float_array");

        nlohmann::json value_json = {1.0, 2.0, 3.0};
        auto value_mapping = std::make_unique<ValueMapping>(value_json);

        std::unordered_map<std::string, std::unique_ptr<Mapping>> entries;
        entries["float_array"] = std::move(value_mapping);
        MapArguments map_args = make_map_arguments(entries, DataType::Float, 1);

        auto dim_return = dim_mapping->map(map_args);

        REQUIRE(!dim_return.empty());
        REQUIRE(dim_return.data_type() == DataType::UInt64);
        REQUIRE(dim_return.rank() == 0);
        REQUIRE(*reinterpret_cast<const int*>(dim_return.buffer()) == 3);
    }

    // Won't work because value mapping doesn't work with array of strings
    // SECTION("Works with string arrays")
    // {
    //     auto dim_mapping = std::make_unique<DimMapping>("string_array");
    //
    //     nlohmann::json value_json = {"hello", "world", "test", "strings"};
    //     auto value_mapping = std::make_unique<ValueMapping>(value_json);
    //
    //     std::unordered_map<std::string, std::unique_ptr<Mapping>> entries;
    //     entries["string_array"] = std::move(value_mapping);
    //     MapArguments map_args = make_map_arguments(std::move(entries), DataType::Unknown, 1);
    //
    //     auto dim_return = dim_mapping->map(map_args);
    //
    //     REQUIRE(!dim_return.empty());
    //     REQUIRE(dim_return.data_type() == DataType::UInt64);
    //     REQUIRE(dim_return.rank() == 0);
    //     REQUIRE(*reinterpret_cast<const int*>(dim_return.buffer()) == 4);
    // }

    SECTION("Works with single element array")
    {
        auto dim_mapping = std::make_unique<DimMapping>("single_element");

        nlohmann::json value_json = {42};
        auto value_mapping = std::make_unique<ValueMapping>(value_json);

        std::unordered_map<std::string, std::unique_ptr<Mapping>> entries;
        entries["single_element"] = std::move(value_mapping);
        MapArguments map_args = make_map_arguments(entries, DataType::UInt64, 1);

        auto dim_return = dim_mapping->map(map_args);

        REQUIRE(!dim_return.empty());
        REQUIRE(dim_return.data_type() == DataType::UInt64);
        REQUIRE(dim_return.rank() == 0);
        REQUIRE(*reinterpret_cast<const int*>(dim_return.buffer()) == 1);
    }

    SECTION("Works with scalar")
    {
        // special case when gives a scalar but in reality needs array of 1 element
        // should still work
        auto dim_mapping = std::make_unique<DimMapping>("single_scalar");

        nlohmann::json value_json = 42;
        auto value_mapping = std::make_unique<ValueMapping>(value_json);

        std::unordered_map<std::string, std::unique_ptr<Mapping>> entries;
        entries["single_scalar"] = std::move(value_mapping);
        MapArguments map_args = make_map_arguments(entries, DataType::UInt64, 0);

        auto dim_return = dim_mapping->map(map_args);

        REQUIRE(!dim_return.empty());
        REQUIRE(dim_return.data_type() == DataType::UInt64);
        REQUIRE(dim_return.rank() == 0);
        REQUIRE(*reinterpret_cast<const int*>(dim_return.buffer()) == 1);
    }
}

TEST_CASE("DimMapping error handling", "[dim_mapping_errors]")
{
    SECTION("Throws error when DIM_PROBE not found")
    {
        auto dim_mapping = std::make_unique<DimMapping>("nonexistent_probe");

        std::unordered_map<std::string, std::unique_ptr<Mapping>> entries;
        MapArguments map_args = make_map_arguments(entries, DataType::UInt64, 0);

        REQUIRE_THROWS_AS(dim_mapping->map(map_args), MappingError);
        REQUIRE_THROWS_WITH(dim_mapping->map(map_args),
                            Catch::Matchers::ContainsSubstring("invalid DIM_PROBE 'nonexistent_probe'"));
    }
}
