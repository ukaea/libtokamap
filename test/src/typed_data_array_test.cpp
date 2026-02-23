#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <span>

#include <vector>

#include "map_types/map_arguments.hpp"
#include "utils/algorithm.hpp"
#include "utils/subset.hpp"

TEST_CASE("Test create array")
{
    SECTION("0d array")
    {
        constexpr int num = 10;
        libtokamap::TypedDataArray array{num};
        REQUIRE(array.rank() == 0);
        REQUIRE(array.size() == 1);
        REQUIRE(array.shape() == std::vector<size_t>{});
        REQUIRE(array.data_type() == libtokamap::DataType::Int32);
        REQUIRE(*reinterpret_cast<int*>(array.buffer()) == num);
    }

    SECTION("1d array")
    {
        constexpr size_t len = 100;
        std::vector<float> vec(len);
        libtokamap::iota(vec, 0);
        libtokamap::TypedDataArray array{vec, {len}};
        REQUIRE(array.rank() == 1);
        REQUIRE(array.size() == len);
        REQUIRE(array.shape() == std::vector<size_t>{len});
        REQUIRE(array.data_type() == libtokamap::DataType::Float);
        REQUIRE(array.to_vector<float>() == vec);
    }

    SECTION("2d array")
    {
        constexpr size_t dim1 = 10;
        constexpr size_t dim2 = 10;
        constexpr size_t len = dim1 * dim2;
        std::vector<float> vec(len);
        libtokamap::iota(vec, 0);
        libtokamap::TypedDataArray array{vec, {dim1, dim2}};
        REQUIRE(array.rank() == 2);
        REQUIRE(array.size() == len);
        REQUIRE(array.shape() == std::vector<size_t>{dim1, dim2});
        REQUIRE(array.data_type() == libtokamap::DataType::Float);
        REQUIRE(array.to_vector<float>() == vec);
    }
}

TEST_CASE("Test array slice")
{
    SECTION("invalid type throws exception")
    {
        constexpr size_t num = 10;
        libtokamap::TypedDataArray array{num};
        REQUIRE_THROWS(array.slice<float>({}));
    }

    SECTION("1d array")
    {
        constexpr size_t len = 100;
        std::vector<float> vec(len);
        libtokamap::iota(vec, 0);
        libtokamap::TypedDataArray array{vec, {len}};

        SECTION("select one element")
        {
            constexpr size_t element = 10;
            std::vector<libtokamap::SubsetInfo> subsets = {libtokamap::SubsetInfo{element, element + 1, 1, len}};
            array.slice<float>(subsets);

            REQUIRE(array.rank() == 0);
            REQUIRE(array.size() == 1);
            REQUIRE(array.shape() == std::vector<size_t>{});
            REQUIRE(array.data_type() == libtokamap::DataType::Float);
            REQUIRE(*reinterpret_cast<float*>(array.buffer()) == element);
        }

        SECTION("select range")
        {
            constexpr size_t range_len = 10;
            constexpr size_t start = 0;
            constexpr size_t stop = start + range_len;
            constexpr size_t stride = 1;
            std::vector<libtokamap::SubsetInfo> subsets = {libtokamap::SubsetInfo{start, stop, stride, len}};
            array.slice<float>(subsets);

            REQUIRE(array.rank() == 1);
            REQUIRE(array.size() == range_len);
            REQUIRE(array.shape() == std::vector<size_t>{range_len});
            REQUIRE(array.data_type() == libtokamap::DataType::Float);

            std::vector<float> expected(range_len);
            libtokamap::iota(expected, 0);
            REQUIRE(array.to_vector<float>() == expected);
        }

        SECTION("select strided range")
        {
            constexpr size_t range_len = 5;
            constexpr size_t start = 0;
            constexpr size_t stride = 2;
            constexpr size_t stop = start + (range_len * stride);
            std::vector<libtokamap::SubsetInfo> subsets = {libtokamap::SubsetInfo{start, stop, stride, len}};
            array.slice<float>(subsets);

            REQUIRE(array.rank() == 1);
            REQUIRE(array.size() == range_len);
            REQUIRE(array.shape() == std::vector<size_t>{range_len});
            REQUIRE(array.data_type() == libtokamap::DataType::Float);

            std::vector<float> expected(range_len);
            libtokamap::iota(expected, 0);
            libtokamap::transform_inplace(expected, [](float val) { return val * stride; });
            REQUIRE(array.to_vector<float>() == expected);
        }
    }

    SECTION("2d array")
    {
        constexpr size_t dim1 = 10;
        constexpr size_t dim2 = 10;
        constexpr size_t len = dim1 * dim2;
        std::vector<float> vec(len);
        libtokamap::iota(vec, 0);
        libtokamap::TypedDataArray array{vec, {dim1, dim2}};

        SECTION("select one element")
        {
            constexpr size_t element1 = 1;
            constexpr size_t element2 = 3;
            std::vector<libtokamap::SubsetInfo> subsets = {
                libtokamap::SubsetInfo{element1, element1 + 1, 1, dim1},
                libtokamap::SubsetInfo{element2, element2 + 1, 1, dim2},
            };
            array.slice<float>(subsets);

            REQUIRE(array.rank() == 0);
            REQUIRE(array.size() == 1);
            REQUIRE(array.shape() == std::vector<size_t>{});
            REQUIRE(array.data_type() == libtokamap::DataType::Float);
            REQUIRE(*reinterpret_cast<float*>(array.buffer()) == (element1 * dim1) + element2);
        }

        SECTION("select 1d slice")
        {
            constexpr size_t element1 = 5;
            std::vector<libtokamap::SubsetInfo> subsets = {
                libtokamap::SubsetInfo{element1, element1 + 1, 1, dim1},
                libtokamap::SubsetInfo{0, std::nullopt, 1, dim2},
            };
            array.slice<float>(subsets);

            REQUIRE(array.rank() == 1);
            REQUIRE(array.size() == 10);
            REQUIRE(array.shape() == std::vector<size_t>{dim2});
            REQUIRE(array.data_type() == libtokamap::DataType::Float);

            std::vector<float> expected(dim2);
            libtokamap::iota(expected, dim1 * element1);
            REQUIRE(array.to_vector<float>() == expected);
        }

        SECTION("select range slice in first dimension")
        {
            constexpr size_t range_len = 3;
            constexpr size_t start = 0;
            constexpr size_t stop = start + range_len;
            constexpr size_t stride = 1;
            std::vector<libtokamap::SubsetInfo> subsets = {
                libtokamap::SubsetInfo{start, stop, stride, dim1},
                libtokamap::SubsetInfo{0, std::nullopt, 1, dim2},
            };
            array.slice<float>(subsets);

            REQUIRE(array.rank() == 2);
            REQUIRE(array.size() == dim2 * range_len);
            REQUIRE(array.shape() == std::vector<size_t>{range_len, dim2});
            REQUIRE(array.data_type() == libtokamap::DataType::Float);

            std::vector<float> expected(dim2 * range_len);
            libtokamap::iota(expected, 0);
            REQUIRE(array.to_vector<float>() == expected);
        }

        SECTION("select range slice in second dimension")
        {
            constexpr size_t range_len = 3;
            constexpr size_t start = 0;
            constexpr size_t stop = start + range_len;
            constexpr size_t stride = 1;
            std::vector<libtokamap::SubsetInfo> subsets = {
                libtokamap::SubsetInfo{0, std::nullopt, 1, dim1},
                libtokamap::SubsetInfo{start, stop, stride, dim2},
            };
            array.slice<float>(subsets);

            REQUIRE(array.rank() == 2);
            REQUIRE(array.size() == dim1 * range_len);
            REQUIRE(array.shape() == std::vector<size_t>{dim1, range_len});
            REQUIRE(array.data_type() == libtokamap::DataType::Float);

            std::vector<float> expected(dim1 * range_len);
            size_t idx = 0;
            for (int i = 0; i < dim1; ++i) {
                for (int j = 0; j < range_len; ++j) {
                    expected[idx] = vec[(dim1 * i) + j];
                    ++idx;
                }
            }
            REQUIRE(array.to_vector<float>() == expected);
        }

        SECTION("select range slice in both dimensions")
        {
            constexpr size_t range1_len = 3;
            constexpr size_t start1 = 0;
            constexpr size_t stop1 = start1 + range1_len;
            constexpr size_t stride1 = 1;

            constexpr size_t range2_len = 2;
            constexpr size_t start2 = 1;
            constexpr size_t stop2 = start2 + range2_len;
            constexpr size_t stride2 = 1;

            std::vector<libtokamap::SubsetInfo> subsets = {
                libtokamap::SubsetInfo{start1, stop1, stride1, dim1},
                libtokamap::SubsetInfo{start2, stop2, stride2, dim2},
            };
            array.slice<float>(subsets);

            REQUIRE(array.rank() == 2);
            REQUIRE(array.size() == range1_len * range2_len);
            REQUIRE(array.shape() == std::vector<size_t>{range1_len, range2_len});
            REQUIRE(array.data_type() == libtokamap::DataType::Float);

            std::vector<float> expected(range1_len * range2_len);
            size_t idx = 0;
            for (int i = start1; i < stop1; ++i) {
                for (int j = start2; j < stop2; ++j) {
                    expected[idx] = vec[(dim1 * i) + j];
                    ++idx;
                }
            }
            REQUIRE(array.to_vector<float>() == expected);
        }
    }
}

TEST_CASE("Test array apply scale and offset")
{
    SECTION("invalid type throws exception")
    {
        constexpr int num = 10;
        libtokamap::TypedDataArray array{num};
        REQUIRE_THROWS(array.apply<float>(1.0, 0.0));
    }

    SECTION("0d array")
    {
        constexpr int num = 10;
        libtokamap::TypedDataArray array{num};

        SECTION("scale")
        {
            array.apply<int>(2.0, 0.0);
            REQUIRE(*reinterpret_cast<int*>(array.buffer()) == 20);
        }

        SECTION("offset")
        {
            array.apply<int>(1.0, 1.0);
            REQUIRE(*reinterpret_cast<int*>(array.buffer()) == 11);
        }

        SECTION("scale and offset")
        {
            array.apply<int>(2.0, 1.0);
            REQUIRE(*reinterpret_cast<int*>(array.buffer()) == 21);
        }
    }

    SECTION("1d array")
    {
        constexpr size_t len = 100;
        std::vector<float> vec(len);
        libtokamap::iota(vec, 0);
        libtokamap::TypedDataArray array{vec, {len}};

        SECTION("scale")
        {
            array.apply<float>(2.0, 0.0);
            std::vector<float> expected;
            libtokamap::transform(vec, expected, [](float value) { return value * 2.0; });
            REQUIRE(array.to_vector<float>() == expected);
        }

        SECTION("offset")
        {
            array.apply<float>(1.0, 1.0);
            std::vector<float> expected;
            libtokamap::transform(vec, expected, [](float value) { return value + 1.0; });
            REQUIRE(array.to_vector<float>() == expected);
        }

        SECTION("scale and offset")
        {
            array.apply<float>(2.0, 1.0);
            std::vector<float> expected;
            libtokamap::transform(vec, expected, [](float value) { return (value * 2.0) + 1.0; });
            REQUIRE(array.to_vector<float>() == expected);
        }
    }

    SECTION("2d array")
    {
        constexpr size_t dim1 = 10;
        constexpr size_t dim2 = 10;
        constexpr size_t len = dim1 * dim2;
        std::vector<float> vec(len);
        libtokamap::iota(vec, 0);
        libtokamap::TypedDataArray array{vec, {dim1, dim2}};

        SECTION("scale")
        {
            array.apply<float>(2.0, 0.0);
            std::vector<float> expected;
            libtokamap::transform(vec, expected, [](float value) { return value * 2.0; });
            REQUIRE(array.to_vector<float>() == expected);
        }

        SECTION("offset")
        {
            array.apply<float>(1.0, 1.0);
            std::vector<float> expected;
            libtokamap::transform(vec, expected, [](float value) { return value + 1.0; });
            REQUIRE(array.to_vector<float>() == expected);
        }

        SECTION("scale and offset")
        {
            array.apply<float>(2.0, 1.0);
            std::vector<float> expected;
            libtokamap::transform(vec, expected, [](float value) { return (value * 2.0) + 1.0; });
            REQUIRE(array.to_vector<float>() == expected);
        }
    }
}
