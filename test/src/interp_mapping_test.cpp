#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "map_types/interp_mapping.hpp"
#include "map_types/value_mapping.hpp"
#include "mapping_test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

using namespace libtokamap;
using namespace libtokamap::test;
using Catch::Matchers::Approx;

namespace
{

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
    // points outside the base are clamped rather than extrapolated
    REQUIRE_THAT(result.to_vector<float>(), Approx(std::vector{10.0F, 10.0F, 15.0F, 25.0F, 30.0F, 40.0F, 40.0F}));
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
