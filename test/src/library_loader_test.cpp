#include <catch2/catch_test_macros.hpp>

#include "exceptions/exceptions.hpp"
#include "map_types/data_source_mapping.hpp"
#include "utils/library_loader.hpp"

TEST_CASE("Data source factories with a matching ABI version load", "[library_loader]")
{
    auto factory = libtokamap::load_data_source_factory(TEST_MATCHING_ABI_FACTORY);
    REQUIRE(factory);
    CHECK(factory({}) == nullptr);
}

TEST_CASE("Data source factories with a different ABI version are rejected", "[library_loader]")
{
    CHECK_THROWS_AS(libtokamap::load_data_source_factory(TEST_MISMATCHED_ABI_FACTORY), libtokamap::ConfigurationError);
}
