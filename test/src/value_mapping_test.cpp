#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ostream>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "map_types/map_arguments.hpp"
#include "map_types/value_mapping.hpp"
#include "utils/ram_cache.hpp"
#include "utils/os_utils.hpp"

namespace
{
// define this _before_ including catch headers
std::ostream& operator<<(std::ostream& out, const std::type_index& value)
{
    out << libtokamap::demangle(value.name());
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
template <> struct StringMaker<std::type_index> {
    static std::string convert(const std::type_index& value)
    {
        int status = 0;
        return libtokamap::demangle(value.name());
    }
};
} // namespace Catch

using namespace libtokamap;

namespace
{

libtokamap::MapArguments make_map_arguments(const std::type_index data_type, const int rank)
{
    static std::unordered_map<std::string, std::unique_ptr<Mapping>> empty_entries;
    static nlohmann::json empty_global_data = nlohmann::json::object();

    constexpr bool trace_enabled = true;
    constexpr bool cache_enabled = false;
    constexpr RamCache* ram_cache = nullptr;

    return MapArguments(empty_entries, empty_global_data, data_type, rank, trace_enabled, cache_enabled, ram_cache);
}

} // namespace

TEST_CASE("ValueMapping can be constructed from JSON", "[value_mapping]")
{
    // Setup test fixture
    nlohmann::json test_json = {{"MAP_TYPE", "VALUE"}, {"VALUE", 42}};

    SECTION("Constructor works with integer value")
    {
        auto mapping = std::make_unique<ValueMapping>(test_json);
        REQUIRE(mapping != nullptr);
    }

    SECTION("Constructor works with float value")
    {
        test_json["VALUE"] = 3.14;
        auto mapping = std::make_unique<ValueMapping>(test_json);
        REQUIRE(mapping != nullptr);
    }

    SECTION("Constructor works with string value")
    {
        test_json["VALUE"] = "test_string";
        auto mapping = std::make_unique<ValueMapping>(test_json);
        REQUIRE(mapping != nullptr);
    }

    SECTION("Constructor works with array value")
    {
        test_json["VALUE"] = {1, 2, 3, 4, 5};
        auto mapping = std::make_unique<ValueMapping>(test_json);
        REQUIRE(mapping != nullptr);
    }

    SECTION("Constructor works with object value")
    {
        test_json["VALUE"] = {{"key1", "value1"}, {"key2", 2}};
        auto mapping = std::make_unique<ValueMapping>(test_json);
        REQUIRE(mapping != nullptr);
    }
}

TEST_CASE("ValueMapping returns expected data for different 0D types", "[value_mapping_0D]")
{

    nlohmann::json test_json = {{"VALUE", 42}};

    SECTION("Integer values are correctly returned")
    {
        test_json["VALUE"] = 42;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = make_map_arguments(std::type_index{typeid(int)}, 0);
        auto array = mapping->map(map_args);

        REQUIRE(!array.empty());
        REQUIRE(array.type_index() == std::type_index{typeid(int)});
        REQUIRE(array.rank() == 0);
        REQUIRE(*reinterpret_cast<const int*>(array.buffer()) == 42);
    }

    SECTION("Negative integer values are correctly returned")
    {
        test_json["VALUE"] = -42;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = make_map_arguments(std::type_index{typeid(int)}, 0);
        auto array = mapping->map(map_args);

        REQUIRE(!array.empty());
        REQUIRE(array.type_index() == std::type_index{typeid(int)});
        REQUIRE(array.rank() == 0);
        REQUIRE(*reinterpret_cast<const int*>(array.buffer()) == -42);
    }

    SECTION("String values are correctly returned")
    {
        test_json["VALUE"] = "Hello World!";

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = make_map_arguments(std::type_index{typeid(const char)}, 1);
        auto array = mapping->map(map_args);

        REQUIRE(!array.empty());
        REQUIRE(array.type_index() == std::type_index{typeid(const char)});
        REQUIRE(array.rank() == 1);
        REQUIRE_THAT(array.buffer(), Catch::Matchers::Equals("Hello World!"));
    }

    SECTION("Float values are correctly returned")
    {
        test_json["VALUE"] = 42.75;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = make_map_arguments(std::type_index{typeid(float)}, 0);
        auto array = mapping->map(map_args);

        REQUIRE(!array.empty());
        REQUIRE(array.type_index() == std::type_index{typeid(float)});
        REQUIRE(array.rank() == 0);
        REQUIRE(*reinterpret_cast<const float*>(array.buffer()) == 42.75);
    }

    SECTION("Negative float values are correctly returned")
    {
        test_json["VALUE"] = -42.75;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = make_map_arguments(std::type_index{typeid(float)}, 0);
        auto array = mapping->map(map_args);

        REQUIRE(!array.empty());
        REQUIRE(array.type_index() == std::type_index{typeid(float)});
        REQUIRE(array.rank() == 0);
        REQUIRE(*reinterpret_cast<const float*>(array.buffer()) == -42.75);
    }
}

TEST_CASE("ValueMapping returns expected data for different 1D types", "[value_mapping_1D]")
{

    nlohmann::json test_json = {{"VALUE", {0, 0, 0}}};

    SECTION("1D integer arrays are correctly returned")
    {
        std::vector<int> test_vector_1d{1, 2, 3, 4};
        test_json["VALUE"] = test_vector_1d;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = make_map_arguments(std::type_index{typeid(int)}, 1);
        auto array = mapping->map(map_args);

        REQUIRE(!array.empty());
        REQUIRE(array.type_index() == std::type_index{typeid(int)});
        REQUIRE(array.rank() == 1);
        const auto* data = reinterpret_cast<const int*>(array.buffer());
        auto vector = std::vector<int>{data, data + array.size()};
        REQUIRE_THAT(vector, Catch::Matchers::RangeEquals(test_vector_1d));
    }

    SECTION("1D negative integer arrays are correctly returned")
    {
        std::vector<int> test_vector_1d{-1, 2, -3, 4};
        test_json["VALUE"] = test_vector_1d;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = make_map_arguments(std::type_index{typeid(int)}, 1);
        auto array = mapping->map(map_args);

        REQUIRE(!array.empty());
        REQUIRE(array.type_index() == std::type_index{typeid(int)});
        REQUIRE(array.rank() == 1);
        const auto* data = reinterpret_cast<const int*>(array.buffer());
        auto vector = std::vector<int>{data, data + array.size()};
        REQUIRE_THAT(vector, Catch::Matchers::RangeEquals(test_vector_1d));
    }

    SECTION("1D float arrays are correctly returned")
    {
        std::vector<float> test_vector_1d{0.1, 0.2, 0.3, 0.4};
        test_json["VALUE"] = test_vector_1d;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = make_map_arguments(std::type_index{typeid(float)}, 1);
        auto array = mapping->map(map_args);

        REQUIRE(!array.empty());
        REQUIRE(array.type_index() == std::type_index{typeid(float)});
        REQUIRE(array.rank() == 1);
        const auto* data = reinterpret_cast<const float*>(array.buffer());
        auto vector = std::vector<float>{data, data + array.size()};
        REQUIRE_THAT(vector, Catch::Matchers::RangeEquals(test_vector_1d));
    }

    SECTION("1D negative float arrays are correctly returned")
    {
        std::vector<float> test_vector_1d{0.1, -0.2, 0.3, -0.4};
        test_json["VALUE"] = test_vector_1d;

        const auto& value_json = test_json.at("VALUE");
        auto mapping = std::make_unique<ValueMapping>(value_json);

        MapArguments map_args = make_map_arguments(std::type_index{typeid(float)}, 1);
        auto array = mapping->map(map_args);

        REQUIRE(!array.empty());
        REQUIRE(array.type_index() == std::type_index{typeid(float)});
        REQUIRE(array.rank() == 1);
        const auto* data = reinterpret_cast<const float*>(array.buffer());
        auto vector = std::vector<float>{data, data + array.size()};
        REQUIRE_THAT(vector, Catch::Matchers::RangeEquals(test_vector_1d));
    }
}
