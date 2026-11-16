// Integration tests for the Python bridge. This executable links only core LibTokaMap: the bridge, Python and NumPy
// are all loaded dynamically through the data source factory loader, as they would be in a C++ application.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <libtokamap.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using Catch::Matchers::ContainsSubstring;

namespace
{

const std::filesystem::path bridge_path = LIBTOKAMAP_TEST_BRIDGE_PATH;
constexpr const char* test_module = "bridge_test_sources";

libtokamap::DataSourceFactory& bridge_factory()
{
    static libtokamap::DataSourceFactory factory = libtokamap::load_data_source_factory(bridge_path);
    return factory;
}

std::unique_ptr<libtokamap::DataSource> make_source(const std::string& class_name,
                                                    libtokamap::DataSourceFactoryArgs args = {})
{
    args["module"] = std::string{test_module};
    args["class_name"] = class_name;
    return bridge_factory()(args);
}

libtokamap::TypedDataArray call(libtokamap::DataSource& source, const libtokamap::DataSourceArgs& args)
{
    static const std::unordered_map<std::string, std::unique_ptr<libtokamap::Mapping>> entries;
    static const nlohmann::json globals = nlohmann::json::object();
    const libtokamap::MapArguments arguments{entries, globals, libtokamap::DataType::Double, 1, false, false, nullptr};
    return source.get(args, arguments, nullptr);
}

libtokamap::TypedDataArray call_kind(const std::string& kind, const std::string& dtype = "float64")
{
    static auto source = make_source("ArraySource");
    return call(*source, {{"kind", kind}, {"dtype", dtype}});
}

std::string as_string(const libtokamap::TypedDataArray& array)
{
    REQUIRE(array.data_type() == libtokamap::DataType::Int8);
    return {array.buffer(), array.size()};
}

template <typename T> void check_values(const libtokamap::TypedDataArray& array, const std::vector<T>& expected)
{
    REQUIRE(array.data_type() == libtokamap::data_type_of<T>());
    REQUIRE(array.to_vector<T>() == expected);
}

template <typename T> void check_arange(const std::string& dtype)
{
    auto array = call_kind("arange", dtype);
    CHECK(array.shape() == std::vector<size_t>{2, 3});
    check_values<T>(array, {0, 1, 2, 3, 4, 5});

    auto swapped = call_kind("byte_swapped", dtype);
    check_values<T>(swapped, {0, 1, 2, 3});
}

} // namespace

TEST_CASE("Python bridge passes constructor and get arguments", "[python_bridge]")
{
    libtokamap::DataSourceFactoryArgs args;
    args["text"] = std::string{"server.example.org"};
    args["port"] = 1234;
    args["ratio"] = 0.5;
    args["enabled"] = true;
    args["nested"] = nlohmann::json{{"list", {1, 2, 3}}, {"none", nullptr}};
    auto source = make_source("EchoSource", args);

    auto result = call(*source, {{"name", "ip"}, {"shot", 42}, {"scale", 1.5}, {"flag", false}});
    auto json = nlohmann::json::parse(as_string(result));

    nlohmann::json expected_config = {{"text", "server.example.org"},
                                      {"port", 1234},
                                      {"ratio", 0.5},
                                      {"enabled", true},
                                      {"nested", {{"list", {1, 2, 3}}, {"none", nullptr}}}};
    CHECK(json["config"] == expected_config);
    CHECK(json["args"] == nlohmann::json{{"name", "ip"}, {"shot", 42}, {"scale", 1.5}, {"flag", false}});
}

TEST_CASE("Python bridge converts NumPy numeric dtypes", "[python_bridge]")
{
    check_arange<int8_t>("int8");
    check_arange<int16_t>("int16");
    check_arange<int32_t>("int32");
    check_arange<int64_t>("int64");
    check_arange<uint8_t>("uint8");
    check_arange<uint16_t>("uint16");
    check_arange<uint32_t>("uint32");
    check_arange<uint64_t>("uint64");
    check_arange<float>("float32");
    check_arange<double>("float64");
}

TEST_CASE("Python bridge converts array layouts", "[python_bridge]")
{
    SECTION("booleans become uint8") { check_values<uint8_t>(call_kind("bool"), {1, 0, 1}); }
    SECTION("zero-dimensional arrays have no shape")
    {
        auto array = call_kind("scalar", "int32");
        CHECK(array.shape().empty());
        check_values<int32_t>(array, {7});
    }
    SECTION("NumPy scalars") { check_values<double>(call_kind("numpy_scalar"), {2.5}); }
    SECTION("empty arrays")
    {
        auto array = call_kind("empty", "float32");
        CHECK(array.size() == 0);
        CHECK(array.shape() == std::vector<size_t>{0});
        CHECK(array.data_type() == libtokamap::DataType::Float);
    }
    SECTION("non-contiguous arrays are gathered in C order")
    {
        auto array = call_kind("non_contiguous", "int64");
        CHECK(array.shape() == std::vector<size_t>{3, 2});
        check_values<int64_t>(array, {0, 2, 4, 6, 8, 10});
    }
    SECTION("Fortran ordered arrays are returned in C order")
    {
        auto array = call_kind("fortran", "float64");
        CHECK(array.shape() == std::vector<size_t>{2, 3});
        check_values<double>(array, {0, 1, 2, 3, 4, 5});
    }
    SECTION("read-only arrays") { check_values<double>(call_kind("read_only"), {0, 1, 2, 3}); }
    SECTION("non-NumPy buffer objects") { check_values<int32_t>(call_kind("memoryview"), {1, 2, 3}); }
    SECTION("object arrays become countable placeholders")
    {
        auto array = call_kind("object");
        CHECK(array.shape() == std::vector<size_t>{3});
        CHECK(array.size() == 3);
        CHECK(array.data_type() == libtokamap::DataType::Int8);
    }
}

TEST_CASE("Python bridge converts strings", "[python_bridge]")
{
    CHECK(as_string(call_kind("str")) == "h\xc3\xa9llo");
    CHECK(as_string(call_kind("unicode_array")) == "h\xc3\xa9llo");
    CHECK(as_string(call_kind("unicode_padded")) == "c");
    CHECK(as_string(call_kind("bytes_array")) == "abc");
}

TEST_CASE("Python bridge rejects unsupported return values", "[python_bridge]")
{
    auto kind = GENERATE(as<std::string>{}, "complex", "unicode_multi", "none", "int");
    CAPTURE(kind);
    CHECK_THROWS_AS(call_kind(kind), libtokamap::PythonCallbackError);

    // The failure must not leave a Python error set or the GIL held
    check_values<double>(call_kind("read_only"), {0, 1, 2, 3});
}

TEST_CASE("Python bridge reports exceptions raised by get", "[python_bridge]")
{
    auto source = make_source("RaisingSource");
    CHECK_THROWS_WITH(call(*source, {}), ContainsSubstring("ValueError: boom from python") &&
                                             ContainsSubstring("bridge_test_sources.RaisingSource.get()") &&
                                             ContainsSubstring("bridge_test_sources.py:"));
    CHECK_THROWS_AS(call(*source, {}), libtokamap::DataSourceError);
}

TEST_CASE("Python bridge reports data source construction failures", "[python_bridge]")
{
    SECTION("missing module")
    {
        libtokamap::DataSourceFactoryArgs args{{"module", std::string{"no_such_module_for_bridge_test"}},
                                               {"class_name", std::string{"Anything"}}};
        CHECK_THROWS_WITH(bridge_factory()(args), ContainsSubstring("ModuleNotFoundError"));
        CHECK_THROWS_AS(bridge_factory()(args), libtokamap::ConfigurationError);
    }
    SECTION("missing class") { CHECK_THROWS_WITH(make_source("NoSuchClass"), ContainsSubstring("AttributeError")); }
    SECTION("not callable") { CHECK_THROWS_WITH(make_source("NOT_A_CLASS"), ContainsSubstring("is not callable")); }
    SECTION("constructor raises")
    {
        CHECK_THROWS_WITH(make_source("FailingConstructor"), ContainsSubstring("RuntimeError: constructor failed"));
    }
    SECTION("no get method")
    {
        CHECK_THROWS_WITH(make_source("NoGetMethod"), ContainsSubstring("no callable 'get' method"));
    }
    SECTION("missing reserved arguments") { CHECK_THROWS_AS(bridge_factory()({}), libtokamap::TokaMapError); }

    // Failed constructions must not affect later ones
    CHECK(as_string(call(*make_source("ArraySource"), {{"kind", "str"}})) == "h\xc3\xa9llo");
}

TEST_CASE("Python bridge calls close() when a data source is destroyed", "[python_bridge]")
{
    auto observer = make_source("ClosableSource", {{"name", std::string{"observer"}}});
    auto closed_count = [&] { return call(*observer, {}).to_vector<int64_t>().at(0); };

    auto before = closed_count();
    auto source = make_source("ClosableSource", {{"name", std::string{"closed"}}});
    source.reset();
    CHECK(closed_count() == before + 1);
}

TEST_CASE("Python bridge data sources can be called from multiple threads", "[python_bridge]")
{
    auto source = make_source("DoublingSource");
    constexpr int num_threads = 8;
    constexpr int calls_per_thread = 50;
    std::atomic<int> failures = 0;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int thread = 0; thread < num_threads; ++thread) {
        threads.emplace_back([&, thread] {
            for (int call_index = 0; call_index < calls_per_thread; ++call_index) {
                int value = (thread * calls_per_thread) + call_index;
                try {
                    auto result = call(*source, {{"value", value}}).to_vector<int64_t>();
                    if (result != std::vector<int64_t>{2 * value}) {
                        ++failures;
                    }
                } catch (...) {
                    ++failures;
                }
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    CHECK(failures == 0);
}

TEST_CASE("Python bridge is usable from MappingHandler configuration", "[python_bridge]")
{
    nlohmann::json config = {
        {"mapping_directory", LIBTOKAMAP_TEST_MAPPINGS_DIR},
        {"schemas_directory", LIBTOKAMAP_TEST_SCHEMAS_DIR},
        {"data_source_factories", {{"python", bridge_path.string()}}},
        {"data_sources",
         {{"PY",
           {{"factory", "python"},
            {"args",
             {{"module", test_module},
              {"class_name", "MappingSource"},
              {"scale", 2.0},
              {"options", {{"offsets", {1, 2, 3}}}}}}}}}},
    };

    libtokamap::MappingHandler handler;
    handler.init(config);

    nlohmann::json attributes = nlohmann::json::object();
    auto value = handler.map("bridge_test", "python/value", libtokamap::DataType::Double, 1, attributes);
    check_values<double>(value, {16.0});

    auto scaled = handler.map("bridge_test", "python/value_scaled", libtokamap::DataType::Double, 1, attributes);
    check_values<double>(scaled, {160.0});

    auto label = handler.map("bridge_test", "python/label", libtokamap::DataType::Int8, 1, attributes);
    CHECK(as_string(label) == "LABEL");
}
