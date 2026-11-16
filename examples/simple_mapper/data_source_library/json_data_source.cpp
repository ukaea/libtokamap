#include "json_data_source.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <ctre/ctre.hpp>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <libtokamap.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace std::string_literals;

namespace
{

std::unique_ptr<libtokamap::DataSource> json_data_source_factory(const libtokamap::DataSourceFactoryArgs& args)
{
    auto data_root = libtokamap::get_arg<std::string>(args, "data_root");
    return std::make_unique<JSONDataSource>(data_root);
}

libtokamap::TypedDataArray parse(const nlohmann::json& value)
{
    if (value.is_number_float()) {
        float number = value;
        return libtokamap::TypedDataArray{number};
    }
    if (value.is_number_integer()) {
        int number = value;
        return libtokamap::TypedDataArray{number};
    }
    if (value.is_string()) {
        std::string string = value;
        return libtokamap::TypedDataArray{string};
    }
    if (value.is_array()) {
        size_t len = value.size();
        if (len == 0) {
            return {};
        }
        if (value[0].is_number_float()) {
            std::vector<float> vector = value;
            return libtokamap::TypedDataArray{vector};
        }
        if (value[0].is_number_integer()) {
            std::vector<int> vector = value;
            return libtokamap::TypedDataArray{vector};
        }
        // returning a vector of strings so we can count them using a DIMENSION map type
        std::vector<std::string> vector(value.size());
        std::ranges::fill(vector.begin(), vector.end(), "<object>"s);
        return libtokamap::TypedDataArray{vector};
    }
    throw libtokamap::JsonError{"invalid json value"};
}

constexpr auto number_re = ctll::fixed_string{R"(\d+)"};

} // namespace

libtokamap::TypedDataArray JSONDataSource::get(const libtokamap::DataSourceArgs& map_args,
                                               const libtokamap::MapArguments& arguments,
                                               libtokamap::RamCache* /*ram_cache*/)
{
    if (!map_args.contains("file_name")) {
        throw libtokamap::ParameterError{"required argument 'file_name' not provided"};
    }
    if (!map_args.contains("signal")) {
        throw libtokamap::ParameterError{"required argument 'signal' not provided"};
    }

    std::string file_name = map_args.at("file_name");
    std::filesystem::path path = m_data_root / file_name;

    if (!m_data.contains(path)) {
        std::ifstream file{path};
        if (!file) {
            throw libtokamap::FileError{"failed to open data file '" + path.string() + "'"};
        }
        m_data[path] = nlohmann::json::parse(file);
    }

    nlohmann::json data = m_data.at(path);

    std::string signal = map_args.at("signal");

    std::deque<std::string> tokens;
    libtokamap::split(tokens, signal, "/");

    nlohmann::json& value = data;
    while (!tokens.empty()) {
        auto& token = tokens.front();
        if (ctre::match<number_re>(token)) {
            int index = std::stoi(token);
            value = data[index];
        } else {
            value = data[token];
        }
        tokens.pop_front();
    }

    auto result = parse(value);
    if (arguments.trace_enabled) {
        std::string version = std::format("{}.{}.{}", NLOHMANN_JSON_VERSION_MAJOR, NLOHMANN_JSON_VERSION_MINOR,
                                          NLOHMANN_JSON_VERSION_PATCH);
        result.set_trace({{"file", path}, {"json_library", {{"name", "nlohmann"}, {"version", version}}}});
    }

    return result;
}

LIBTOKAMAP_EXPORT_FACTORY_ABI_VERSION()

void LibTokaMapFactoryLoader(libtokamap::FactoryEntryInterface& factory)
{
    factory.function = json_data_source_factory;
}
