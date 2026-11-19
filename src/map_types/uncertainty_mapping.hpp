#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/typed_data_array.hpp"

namespace libtokamap
{

/// absolute, at every point
struct AbsoluteUncertainty {
    static constexpr std::string_view name = "ABSOLUTE";
    double absolute;
};

/// relative * |x|, relative as a fraction
struct RelativeUncertainty {
    static constexpr std::string_view name = "RELATIVE";
    double relative;
};

/// max(absolute, relative * |x|), an absolute floor on a relative uncertainty
struct MaxAbsRelUncertainty {
    static constexpr std::string_view name = "MAX_ABS_REL";
    double absolute;
    double relative;
};

using UncertaintyModel = std::variant<AbsoluteUncertainty, RelativeUncertainty, MaxAbsRelUncertainty>;

/**
 * @brief Build an uncertainty model from the JSON of an UNCERTAINTY mapping
 *
 * Reads `model_type` and the parameters it needs (`absolute` and/or `relative`).
 * Parameters may be numbers or template strings rendered against the group
 * attributes. Throws ConfigurationError, naming the mapping, for an unknown
 * `model_type`, a missing or unused parameter, or a parameter that is negative
 * or not a number.
 */
UncertaintyModel parse_uncertainty_model(const nlohmann::json& value, const nlohmann::json& group_attributes,
                                         const std::string& mapping_name);

/**
 * @brief Per point uncertainty on a reference signal
 *
 * The result has the same shape and floating point type (float or double) as
 * the reference. The model is chosen when the mapping is loaded, so there is no
 * per point dispatch when it is mapped.
 */
class UncertaintyMapping : public Mapping
{
  public:
    UncertaintyMapping() = delete;
    UncertaintyMapping(std::string reference, UncertaintyModel model)
        : m_reference{std::move(reference)}, m_model{model} {};

    [[nodiscard]] TypedDataArray map(const MapArguments& arguments) const override;

  private:
    std::string m_reference;
    UncertaintyModel m_model;
};

} // namespace libtokamap
