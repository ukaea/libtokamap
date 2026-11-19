#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <nlohmann/json.hpp>
#include <utility>
#include <variant>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "map_types/uncertainty_mapping.hpp"
#include "map_types/value_mapping.hpp"
#include "mapping_test_helpers.hpp"
#include "utils/typed_data_array.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

namespace
{

constexpr auto reference_name = "magnetics/coil[#]/uncertainty_reference";

libtokamap::test::MapEntries make_entries(const nlohmann::json& reference)
{
    libtokamap::test::MapEntries entries;
    entries[reference_name] = std::make_unique<libtokamap::ValueMapping>(reference);
    return entries;
}

/// returns a fixed array, for references ValueMapping cannot produce (double, rank >= 2)
class FixedMapping : public libtokamap::Mapping
{
  public:
    FixedMapping(std::vector<double> values, std::vector<size_t> shape)
        : m_values{std::move(values)}, m_shape{std::move(shape)}
    {
    }
    [[nodiscard]] libtokamap::TypedDataArray map(const libtokamap::MapArguments& /*arguments*/) const override
    {
        return libtokamap::TypedDataArray{m_values, m_shape};
    }

  private:
    std::vector<double> m_values;
    std::vector<size_t> m_shape;
};

libtokamap::UncertaintyModel parse(const nlohmann::json& value, const nlohmann::json& group_attributes = {})
{
    return libtokamap::parse_uncertainty_model(value, group_attributes, "test_mapping");
}

} // namespace

TEST_CASE("UncertaintyMapping computes per point uncertainties", "[uncertainty_mapping]")
{
    // negative values check the relative uncertainty uses |x|
    auto entries = make_entries({-20.0, -1.0, 0.0, 1.0, 20.0});
    const libtokamap::MapArguments map_args = libtokamap::test::make_map_arguments(entries);

    SECTION("absolute")
    {
        const libtokamap::UncertaintyMapping mapping{reference_name, libtokamap::AbsoluteUncertainty{0.5}};
        REQUIRE_THAT(mapping.map(map_args).to_vector<float>(),
                     Catch::Matchers::Approx(std::vector{0.5F, 0.5F, 0.5F, 0.5F, 0.5F}));
    }

    SECTION("relative")
    {
        const libtokamap::UncertaintyMapping mapping{reference_name, libtokamap::RelativeUncertainty{0.1}};
        REQUIRE_THAT(mapping.map(map_args).to_vector<float>(),
                     Catch::Matchers::Approx(std::vector{2.0F, 0.1F, 0.0F, 0.1F, 2.0F}));
    }

    SECTION("max of absolute and relative")
    {
        // absolute wins for small |x|, relative wins for large |x|
        const libtokamap::UncertaintyMapping mapping{
            reference_name, libtokamap::MaxAbsRelUncertainty{.absolute = 0.5, .relative = 0.1}};
        REQUIRE_THAT(mapping.map(map_args).to_vector<float>(),
                     Catch::Matchers::Approx(std::vector{2.0F, 0.5F, 0.5F, 0.5F, 2.0F}));
    }
}

TEST_CASE("UncertaintyMapping keeps missing (NaN) points missing", "[uncertainty_mapping]")
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    libtokamap::test::MapEntries entries;
    entries[reference_name] = std::make_unique<FixedMapping>(std::vector{-20.0, nan, 1.0}, std::vector<size_t>{3});
    const libtokamap::MapArguments map_args = libtokamap::test::make_map_arguments(entries);

    SECTION("relative")
    {
        const libtokamap::UncertaintyMapping mapping{reference_name, libtokamap::RelativeUncertainty{0.1}};
        const auto result = mapping.map(map_args).to_vector<double>();
        REQUIRE(result[0] == Catch::Approx(2.0));
        REQUIRE(std::isnan(result[1]));
        REQUIRE(result[2] == Catch::Approx(0.1));
    }

    SECTION("max of absolute and relative")
    {
        // std::max(absolute, NaN) would give absolute
        const libtokamap::UncertaintyMapping mapping{
            reference_name, libtokamap::MaxAbsRelUncertainty{.absolute = 0.5, .relative = 0.1}};
        const auto result = mapping.map(map_args).to_vector<double>();
        REQUIRE(result[0] == Catch::Approx(2.0));
        REQUIRE(std::isnan(result[1]));
        REQUIRE(result[2] == Catch::Approx(0.5));
    }
}

TEST_CASE("UncertaintyMapping keeps the reference type and shape", "[uncertainty_mapping]")
{
    libtokamap::test::MapEntries entries;
    entries[reference_name] =
        std::make_unique<FixedMapping>(std::vector{1.0, -2.0, 3.0, -4.0, 5.0, -6.0}, std::vector<size_t>{2, 3});
    const libtokamap::MapArguments map_args = libtokamap::test::make_map_arguments(entries);
    const libtokamap::UncertaintyMapping mapping{reference_name, libtokamap::RelativeUncertainty{0.5}};

    const auto result = mapping.map(map_args);
    REQUIRE(result.data_type() == libtokamap::DataType::Double);
    REQUIRE(result.shape() == std::vector<size_t>{2, 3});
    REQUIRE_THAT(result.to_vector<double>(), Catch::Matchers::Approx(std::vector{0.5, 1.0, 1.5, 2.0, 2.5, 3.0}));
}

TEST_CASE("UncertaintyMapping rejects references it cannot use", "[uncertainty_mapping_errors]")
{
    SECTION("non floating point reference")
    {
        auto entries = make_entries({10, 20});
        const libtokamap::MapArguments map_args = libtokamap::test::make_map_arguments(entries);
        const libtokamap::UncertaintyMapping mapping{reference_name, libtokamap::AbsoluteUncertainty{0.5}};

        REQUIRE_THROWS_AS(mapping.map(map_args), libtokamap::DataTypeError);
        REQUIRE_THROWS_WITH(mapping.map(map_args),
                            Catch::Matchers::ContainsSubstring("must be float or double") &&
                                Catch::Matchers::ContainsSubstring("write the numbers as decimals"));
    }

    SECTION("missing reference mapping")
    {
        auto entries = make_entries({10.0, 20.0});
        const libtokamap::MapArguments map_args = libtokamap::test::make_map_arguments(entries);
        const libtokamap::UncertaintyMapping mapping{"not_a_mapping", libtokamap::AbsoluteUncertainty{0.5}};

        REQUIRE_THROWS_AS(mapping.map(map_args), libtokamap::MappingError);
    }
}

TEST_CASE("parse_uncertainty_model builds the model from JSON", "[uncertainty_mapping_parse]")
{
    SECTION("absolute")
    {
        const auto model = parse({{"model_type", "ABSOLUTE"}, {"absolute", 0.5}});
        REQUIRE(std::get<libtokamap::AbsoluteUncertainty>(model).absolute == 0.5);
    }

    SECTION("relative")
    {
        const auto model = parse({{"model_type", "RELATIVE"}, {"relative", 0.1}});
        REQUIRE(std::get<libtokamap::RelativeUncertainty>(model).relative == 0.1);
    }

    SECTION("max of absolute and relative")
    {
        const auto model = parse({{"model_type", "MAX_ABS_REL"}, {"absolute", 0.5}, {"relative", 0.1}});
        const auto& max_abs_rel = std::get<libtokamap::MaxAbsRelUncertainty>(model);
        REQUIRE(max_abs_rel.absolute == 0.5);
        REQUIRE(max_abs_rel.relative == 0.1);
    }

    SECTION("template string rendered from group attributes")
    {
        const auto model = parse({{"model_type", "ABSOLUTE"}, {"absolute", "{{ absolute_uncertainty }}"}},
                                 {{"absolute_uncertainty", 3.0}});
        REQUIRE(std::get<libtokamap::AbsoluteUncertainty>(model).absolute == 3.0);
    }
}

TEST_CASE("parse_uncertainty_model rejects invalid JSON", "[uncertainty_mapping_parse_errors]")
{
    using Catch::Matchers::ContainsSubstring;

    SECTION("unknown model_type")
    {
        REQUIRE_THROWS_WITH(parse({{"model_type", "QUADRATURE"}, {"absolute", 0.1}}),
                            ContainsSubstring("Unknown model_type 'QUADRATURE'") && ContainsSubstring("test_mapping"));
    }

    SECTION("missing model_type") { REQUIRE_THROWS_AS(parse({{"absolute", 0.1}}), libtokamap::ConfigurationError); }

    SECTION("missing parameter")
    {
        REQUIRE_THROWS_WITH(parse({{"model_type", "MAX_ABS_REL"}, {"absolute", 0.1}}),
                            ContainsSubstring("Required relative"));
    }

    SECTION("unused parameter")
    {
        REQUIRE_THROWS_WITH(parse({{"model_type", "ABSOLUTE"}, {"absolute", 0.1}, {"relative", 0.05}}),
                            ContainsSubstring("relative is not used by model_type ABSOLUTE"));
        REQUIRE_THROWS_WITH(parse({{"model_type", "RELATIVE"}, {"relative", 0.1}, {"absolute", 0.05}}),
                            ContainsSubstring("absolute is not used by model_type RELATIVE"));
    }

    SECTION("negative parameter")
    {
        REQUIRE_THROWS_WITH(parse({{"model_type", "RELATIVE"}, {"relative", -0.05}}), ContainsSubstring(">= 0"));
        REQUIRE_THROWS_WITH(parse({{"model_type", "ABSOLUTE"}, {"absolute", "-1.0"}}), ContainsSubstring(">= 0"));
    }

    SECTION("template string that is not a number")
    {
        REQUIRE_THROWS_WITH(parse({{"model_type", "ABSOLUTE"}, {"absolute", "{{ name }}"}}, {{"name", "abc"}}),
                            ContainsSubstring("'abc' is not a number"));
        REQUIRE_THROWS_WITH(parse({{"model_type", "ABSOLUTE"}, {"absolute", "0.1abc"}}),
                            ContainsSubstring("is not a number"));
    }

    SECTION("parameter of the wrong JSON type")
    {
        REQUIRE_THROWS_AS(parse({{"model_type", "ABSOLUTE"}, {"absolute", true}}), libtokamap::ConfigurationError);
    }
}
