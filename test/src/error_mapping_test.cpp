#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "map_types/error_mapping.hpp"
#include "map_types/value_mapping.hpp"
#include "mapping_test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

namespace
{

libtokamap::test::MapEntries make_entries(const nlohmann::json& reference)
{
    libtokamap::test::MapEntries entries;
    entries["magnetics/coil[#]/error_reference"] = std::make_unique<libtokamap::ValueMapping>(reference);
    return entries;
}

} // namespace

TEST_CASE("ErrorMapping computes per point errors", "[error_mapping]")
{
    // negative values check the relative error uses |x|
    auto entries = make_entries({-20.0, -1.0, 0.0, 1.0, 20.0});
    const libtokamap::MapArguments map_args = libtokamap::test::make_map_arguments(entries);

    SECTION("absolute")
    {
        const libtokamap::ErrorMapping mapping{"magnetics/coil[#]/error_reference", libtokamap::ErrorType::ABSOLUTE,
                                               0.5, std::nullopt};
        REQUIRE_THAT(mapping.map(map_args).to_vector<float>(),
                     Catch::Matchers::Approx(std::vector{0.5F, 0.5F, 0.5F, 0.5F, 0.5F}));
    }

    SECTION("relative")
    {
        const libtokamap::ErrorMapping mapping{"magnetics/coil[#]/error_reference", libtokamap::ErrorType::RELATIVE,
                                               std::nullopt, 0.1};
        REQUIRE_THAT(mapping.map(map_args).to_vector<float>(),
                     Catch::Matchers::Approx(std::vector{2.0F, 0.1F, 0.0F, 0.1F, 2.0F}));
    }

    SECTION("max of absolute and relative")
    {
        // absolute wins for small |x|, relative wins for large |x|
        const libtokamap::ErrorMapping mapping{"magnetics/coil[#]/error_reference",
                                               libtokamap::ErrorType::MAX_ABS_REL, 0.5, 0.1};
        REQUIRE_THAT(mapping.map(map_args).to_vector<float>(),
                     Catch::Matchers::Approx(std::vector{2.0F, 0.5F, 0.5F, 0.5F, 2.0F}));
    }
}

TEST_CASE("ErrorMapping rejects references it cannot use", "[error_mapping_errors]")
{
    SECTION("non floating point reference")
    {
        auto entries = make_entries({10, 20});
        const libtokamap::MapArguments map_args = libtokamap::test::make_map_arguments(entries);
        const libtokamap::ErrorMapping mapping{"magnetics/coil[#]/error_reference", libtokamap::ErrorType::ABSOLUTE,
                                               0.5, std::nullopt};

        REQUIRE_THROWS_AS(mapping.map(map_args), libtokamap::DataTypeError);
        REQUIRE_THROWS_WITH(mapping.map(map_args), Catch::Matchers::ContainsSubstring("must be floating point"));
    }

    SECTION("missing reference mapping")
    {
        auto entries = make_entries({10.0, 20.0});
        const libtokamap::MapArguments map_args = libtokamap::test::make_map_arguments(entries);
        const libtokamap::ErrorMapping mapping{"not_a_mapping", libtokamap::ErrorType::ABSOLUTE, 0.5, std::nullopt};

        REQUIRE_THROWS_AS(mapping.map(map_args), libtokamap::MappingError);
    }
}
