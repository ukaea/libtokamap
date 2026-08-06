#include <cstddef>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "map_types/base_mapping.hpp"
#include "map_types/interp_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "map_types/value_mapping.hpp"
#include "utils/typed_data_array.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace libtokamap;

namespace
{

using MapEntries = std::unordered_map<std::string, std::unique_ptr<Mapping>>;

MapArguments make_map_arguments(MapEntries& entries)
{
    static nlohmann::json empty_global_data = nlohmann::json::object();

    constexpr bool trace_enabled = false;
    constexpr bool cache_enabled = false;
    constexpr RamCache* ram_cache = nullptr;

    return MapArguments(entries, empty_global_data, DataType::Float, 1, trace_enabled, cache_enabled, ram_cache);
}

MapEntries make_entries(const nlohmann::json& signal, const nlohmann::json& time, const nlohmann::json& new_time)
{
    MapEntries entries;
    entries["signal"] = std::make_unique<ValueMapping>(signal);
    entries["time"] = std::make_unique<ValueMapping>(time);
    entries["new_time"] = std::make_unique<ValueMapping>(new_time);
    return entries;
}

InterpMapping make_interp_mapping()
{
    return InterpMapping{"signal", "time", "new_time", InterpType::LINEAR};
}

} // namespace

TEST_CASE("InterpMapping interpolates onto a new time base", "[interp_mapping]")
{
    // throw a few situations, on the nose, out of bounds, inbetween, all the rest
    auto entries = make_entries(
        {10.0, 20.0, 30.0, 40.0},
        {0.0, 1.0, 2.0, 3.0},
        {-1.0, 0.0, 0.5, 1.5, 2.0, 3.0, 5.0}
    );
    MapArguments map_args = make_map_arguments(entries);

    const auto result = make_interp_mapping().map(map_args);

    REQUIRE(result.rank() == 1);
    REQUIRE(result.size() == 7);
    REQUIRE(result.data_type() == DataType::Float);

    // points outside the base are clamped rather than extrapolated
    const std::vector<float> expected{10.0F, 10.0F, 15.0F, 25.0F, 30.0F, 40.0F, 40.0F};
    const auto actual = result.to_vector<float>();
    for (size_t idx = 0; idx < expected.size(); ++idx) {
        INFO("index " << idx);
        REQUIRE(actual[idx] == Catch::Approx(expected[idx]));
    }
}

TEST_CASE("InterpMapping rejects arguments it cannot interpolate", "[interp_mapping_errors]")
{
    SECTION("non floating point data")
    {
        auto entries = make_entries({10, 20}, {0.0, 1.0}, {0.5});
        MapArguments map_args = make_map_arguments(entries);

        REQUIRE_THROWS_AS(make_interp_mapping().map(map_args), DataTypeError);
        REQUIRE_THROWS_WITH(make_interp_mapping().map(map_args),
                            Catch::Matchers::ContainsSubstring("must be floating point"));
    }

    SECTION("input and base of different lengths")
    {
        auto entries = make_entries({10.0, 20.0, 30.0}, {0.0, 1.0}, {0.5});
        MapArguments map_args = make_map_arguments(entries);

        REQUIRE_THROWS_AS(make_interp_mapping().map(map_args), ProcessingError);
    }

    SECTION("base that is not strictly increasing")
    {
        auto entries = make_entries({10.0, 20.0, 30.0, 40.0}, {0.0, 2.0, 1.0, 3.0}, {0.5});
        MapArguments map_args = make_map_arguments(entries);

        REQUIRE_THROWS_AS(make_interp_mapping().map(map_args), ProcessingError);
        REQUIRE_THROWS_WITH(make_interp_mapping().map(map_args),
                            Catch::Matchers::ContainsSubstring("must be strictly increasing"));
    }

    SECTION("a base with only one point")
    {
        auto entries = make_entries({10.0}, {0.0}, {0.5});
        MapArguments map_args = make_map_arguments(entries);

        REQUIRE_THROWS_AS(make_interp_mapping().map(map_args), ProcessingError);
        REQUIRE_THROWS_WITH(make_interp_mapping().map(map_args),
                            Catch::Matchers::ContainsSubstring("must have at least 2 points"));
    }
}
