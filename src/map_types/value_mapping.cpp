#include "value_mapping.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <inja/inja.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/profiler.hpp"
#include "utils/render.hpp"
#include "utils/typed_data_array.hpp"

namespace
{

template <typename T> T string_to(const std::string& string, size_t* ptr);

template <> float string_to<float>(const std::string& string, size_t* ptr) { return std::stof(string, ptr); }
template <> double string_to<double>(const std::string& string, size_t* ptr) { return std::stod(string, ptr); }
template <> int32_t string_to<int32_t>(const std::string& string, size_t* ptr) { return std::stoi(string, ptr); }
// template <> int64_t string_to<int64_t>(const std::string& string, size_t* ptr) { return std::stol(string, ptr); }

template <typename T> std::string name();

template <> std::string name<float>() { return "float"; }
template <> std::string name<double>() { return "double"; }
template <> std::string name<int32_t>() { return "int32_t"; }
// template <> std::string name<int64_t>() { return "int64_t"; }

template <typename T>
T try_convert(const std::string& input)
    requires std::is_scalar_v<T>
{
    size_t end = 0;
    try {
        T result = string_to<T>(input, &end);
        if (end != input.size()) {
            throw std::invalid_argument("input is not a " + name<T>());
        }
        return result;
    } catch (std::invalid_argument&) {
        throw std::invalid_argument("input is not a " + name<T>());
    }
}

template <typename ARRAY_T>
std::vector<std::remove_all_extents_t<ARRAY_T>> try_convert(const std::string& input)
    requires std::is_array_v<ARRAY_T>
{
    using T = std::remove_extent_t<ARRAY_T>;
    try {
        if (input.empty()) {
            throw std::invalid_argument("input is empty");
        }
        if (input.front() != '[') {
            throw std::invalid_argument("input is not a " + name<T>() + "[]");
        }
        if (input.back() != ']') {
            throw std::invalid_argument("input is not a " + name<T>() + "[]");
        }

        auto trimmed = input.substr(1, input.size() - 2);

        std::vector<T> result;
        auto pos = trimmed.find(',');
        size_t start = 0U;
        while (pos != std::string::npos) {
            auto sub = trimmed.substr(start, pos - start);
            start = pos + 1;
            pos = trimmed.find(',', start);
            result.emplace_back(try_convert<T>(sub));
        }
        auto rem = trimmed.substr(start);
        result.emplace_back(try_convert<T>(rem));
        return result;
    } catch (std::invalid_argument&) {
        throw std::invalid_argument("input is not a " + name<T>() + "[]");
    }
}

libtokamap::TypedDataArray type_deduce_array(const nlohmann::json& temp_val)
{
    switch (temp_val.front().type()) {
        case nlohmann::json::value_t::number_float:
            return libtokamap::TypedDataArray{temp_val.get<std::vector<float>>()};
        case nlohmann::json::value_t::number_integer:
            return libtokamap::TypedDataArray{temp_val.get<std::vector<int>>()};
        case nlohmann::json::value_t::number_unsigned:
            return libtokamap::TypedDataArray{temp_val.get<std::vector<unsigned int>>()};
        default:
            return {};
    }
}

libtokamap::TypedDataArray type_deduce_primitive(const nlohmann::json& temp_val, const nlohmann::json& global_data,
                                                 libtokamap::DataType data_type, int rank)
{
    using libtokamap::DataType;
    switch (temp_val.type()) {
        case nlohmann::json::value_t::number_float:
            return libtokamap::TypedDataArray{temp_val.get<float>()};
        case nlohmann::json::value_t::number_integer:
            return libtokamap::TypedDataArray{temp_val.get<int>()};
        case nlohmann::json::value_t::number_unsigned:
            return libtokamap::TypedDataArray{temp_val.get<unsigned int>()};
        case nlohmann::json::value_t::boolean:
            return libtokamap::TypedDataArray{temp_val.get<bool>()};
        case nlohmann::json::value_t::string: {
            // Handle string
            std::string const rendered_string = libtokamap::render(temp_val.get<std::string>(), global_data);

            // try to convert to integer
            // catch exception - output as string
            // inja templating may replace with number
            try {
                if (rank == 0) {
                    switch (data_type) {
                        case DataType::Int32:
                            return libtokamap::TypedDataArray{try_convert<int32_t>(rendered_string)};
                        case DataType::Float:
                            return libtokamap::TypedDataArray{try_convert<float>(rendered_string)};
                        case DataType::Double:
                            return libtokamap::TypedDataArray{try_convert<double>(rendered_string)};
                        default:
                            return libtokamap::TypedDataArray{rendered_string};
                    }
                } else {
                    switch (data_type) {
                        case DataType::Int32:
                            return libtokamap::TypedDataArray{try_convert<int32_t[]>(rendered_string)};
                        case DataType::Float:
                            return libtokamap::TypedDataArray{try_convert<float[]>(rendered_string)};
                        case DataType::Double:
                            return libtokamap::TypedDataArray{try_convert<double[]>(rendered_string)};
                        default:
                            return libtokamap::TypedDataArray{rendered_string};
                    }
                }
            } catch (const std::invalid_argument&) {
                // UDA_LOG(UDA_LOG_DEBUG,
                //         "ValueMapping::map failure to convert"
                //         "string to int in mapping : %s\n",
                //         e.what());
                return libtokamap::TypedDataArray{rendered_string};
            }
            break;
        }
        default:
            throw libtokamap::JsonError{"unknown json type"};
    }

#ifndef WIN32
    return {};
#endif
}

} // namespace

libtokamap::TypedDataArray libtokamap::ValueMapping::map(const MapArguments& arguments) const
{
    LIBTOKAMAP_PROFILER(profiler);

    libtokamap::TypedDataArray result;

    const auto temp_val = m_value;
    if (temp_val.is_discarded() or temp_val.is_binary() or temp_val.is_null()) {
        throw libtokamap::JsonError{"map unrecognised json value type"};
    }

    if (temp_val.is_array()) {
        // Check all members of array are numbers
        // (Add array of strings if necessary)
        const bool all_number =
            std::ranges::all_of(temp_val, [](const nlohmann::json& els) { return els.is_number(); });

        // deduce type if true
        if (all_number) {
            result = type_deduce_array(temp_val);
        }

    } else if (temp_val.is_primitive()) {
        result = type_deduce_primitive(temp_val, arguments.global_data, arguments.data_type, arguments.rank);
    } else {
        throw libtokamap::ProcessingError{"map not structured or primitive"};
    }

    if (arguments.trace_enabled) {
        result.set_trace({{"map_type", "value"}, {"value", m_value}});
    }
    return result;
}
