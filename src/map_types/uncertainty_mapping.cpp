#include "map_types/uncertainty_mapping.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include "exceptions/exceptions.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/render.hpp"
#include "utils/typed_data_array.hpp"

namespace
{

using libtokamap::AbsoluteUncertainty;
using libtokamap::ConfigurationError;
using libtokamap::MaxAbsRelUncertainty;
using libtokamap::RelativeUncertainty;

/**
 * @brief Read a non-negative uncertainty parameter, rendering it first if it
 * is a template string
 */
double parse_parameter(const nlohmann::json& value, const char* key, const nlohmann::json& group_attributes,
                       const std::string& mapping_name)
{
    const auto context = std::string{" in UNCERTAINTY mapping '"} + mapping_name + "'";
    if (!value.contains(key)) {
        throw ConfigurationError{"Required " + std::string{key} + " argument not provided" + context};
    }

    const auto& param = value.at(key);
    double result = 0.0;
    if (param.is_number()) {
        result = param.get<double>();
    } else if (param.is_string()) {
        const auto rendered = libtokamap::render(param.get<std::string>(), group_attributes);
        size_t consumed = 0;
        try {
            result = std::stod(rendered, &consumed);
        } catch (const std::logic_error&) { // invalid_argument and out_of_range
            consumed = 0;
        }
        if (consumed == 0 || rendered.find_first_not_of(" \t\n", consumed) != std::string::npos) {
            throw ConfigurationError{std::string{key} + " '" + rendered + "' is not a number" + context};
        }
    } else {
        throw ConfigurationError{std::string{key} + " must be a number or template string" + context};
    }

    // negated so that NaN is also rejected
    if (!(result >= 0.0) || std::isinf(result)) {
        throw ConfigurationError{std::string{key} + " must be finite and >= 0" + context};
    }
    return result;
}

void reject_parameter(const nlohmann::json& value, const char* key, const std::string& model_type,
                      const std::string& mapping_name)
{
    if (value.contains(key)) {
        throw ConfigurationError{std::string{key} + " is not used by model_type " + model_type +
                                 " in UNCERTAINTY mapping '" + mapping_name + "'"};
    }
}

// One overload per model, each over the whole array, in place. The loops are
// branch free (the NaN check is a select) so they vectorise.

template <typename T> void apply(const AbsoluteUncertainty& model, std::span<T> values)
{
    std::ranges::fill(values, static_cast<T>(model.absolute));
}

template <typename T> void apply(const RelativeUncertainty& model, std::span<T> values)
{
    const auto relative = static_cast<T>(model.relative);
    for (auto& value : values) {
        value = relative * std::abs(value);
    }
}

template <typename T> void apply(const MaxAbsRelUncertainty& model, std::span<T> values)
{
    const auto absolute = static_cast<T>(model.absolute);
    const auto relative = static_cast<T>(model.relative);
    for (auto& value : values) {
        // std::max(absolute, NaN) is absolute, keep NaN so missing points stay missing
        value = std::isnan(value) ? value : std::max(absolute, relative * std::abs(value));
    }
}

} // namespace

libtokamap::UncertaintyModel libtokamap::parse_uncertainty_model(const nlohmann::json& value,
                                                                 const nlohmann::json& group_attributes,
                                                                 const std::string& mapping_name)
{
    if (!value.contains("model_type") || !value.at("model_type").is_string()) {
        throw ConfigurationError{"Required model_type argument not provided in UNCERTAINTY mapping '" + mapping_name +
                                 "'"};
    }
    const auto model_type = value.at("model_type").get<std::string>();

    if (model_type == AbsoluteUncertainty::name) {
        reject_parameter(value, "relative", model_type, mapping_name);
        return AbsoluteUncertainty{parse_parameter(value, "absolute", group_attributes, mapping_name)};
    }
    if (model_type == RelativeUncertainty::name) {
        reject_parameter(value, "absolute", model_type, mapping_name);
        return RelativeUncertainty{parse_parameter(value, "relative", group_attributes, mapping_name)};
    }
    if (model_type == MaxAbsRelUncertainty::name) {
        return MaxAbsRelUncertainty{.absolute = parse_parameter(value, "absolute", group_attributes, mapping_name),
                                    .relative = parse_parameter(value, "relative", group_attributes, mapping_name)};
    }
    throw ConfigurationError{"Unknown model_type '" + model_type + "' in UNCERTAINTY mapping '" + mapping_name +
                             "', expected ABSOLUTE, RELATIVE or MAX_ABS_REL"};
}

libtokamap::TypedDataArray libtokamap::UncertaintyMapping::map(const libtokamap::MapArguments& arguments) const
{
    if (!arguments.entries.contains(m_reference)) {
        throw libtokamap::MappingError{"Mapping '" + m_reference + "' referenced by UNCERTAINTY mapping not found"};
    }
    // map() always hands back an owned array (cache hits are clones), so the
    // uncertainty is written over the reference values, keeping type and shape
    auto result = arguments.entries.at(m_reference)->map(arguments);

    if (result.empty()) {
        throw libtokamap::ProcessingError{"UNCERTAINTY reference '" + m_reference + "' is empty"};
    }

    switch (result.data_type()) {
        case DataType::Float:
            std::visit([&](const auto& model) { apply(model, result.span<float>()); }, m_model);
            break;
        case DataType::Double:
            std::visit([&](const auto& model) { apply(model, result.span<double>()); }, m_model);
            break;
        default:
            throw libtokamap::DataTypeError{
                "UNCERTAINTY reference '" + m_reference + "' must be float or double, got " +
                data_type_name(result.data_type()) +
                ". For a VALUE mapping write the numbers as decimals, eg. [10.0, 20.0] not [10, 20]"};
    }

    if (arguments.trace_enabled) {
        auto model_type = std::visit([](const auto& model) { return std::string{model.name}; }, m_model);
        result.set_trace({{"map_type", "uncertainty"},
                          {"model_type", std::move(model_type)},
                          {"reference", {{m_reference, result.trace()}}}});
    }
    return result;
}
