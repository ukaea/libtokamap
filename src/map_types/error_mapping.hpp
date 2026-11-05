#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/typed_data_array.hpp"

namespace libtokamap
{

enum class ErrorType : short { UNKNOWN, ABSOLUTE, RELATIVE, MAX_ABS_REL };

NLOHMANN_JSON_SERIALIZE_ENUM(ErrorType, {{ErrorType::UNKNOWN, nullptr},
                                         {ErrorType::ABSOLUTE, "ABSOLUTE"},
                                         {ErrorType::RELATIVE, "RELATIVE"},
                                         {ErrorType::MAX_ABS_REL, "MAX_ABS_REL"}})

/**
 * @brief Per point error on a reference signal
 *
 * ABSOLUTE    -> absolute
 * RELATIVE    -> relative * |x|
 * MAX_ABS_REL -> max(absolute, relative * |x|)
 *
 * The result has the same shape as the reference. Parameters not used by the
 * error type are expected to be empty, this is checked when the mapping is loaded
 */
class ErrorMapping : public Mapping
{
  public:
    ErrorMapping() = delete;
    ErrorMapping(std::string reference, ErrorType error_type, std::optional<double> absolute,
                 std::optional<double> relative)
        : m_reference{std::move(reference)}, m_error_type{error_type}, m_absolute{absolute}, m_relative{relative} {};

    [[nodiscard]] TypedDataArray map(const MapArguments& arguments) const override;

  private:
    std::string m_reference;
    ErrorType m_error_type;
    std::optional<double> m_absolute;
    std::optional<double> m_relative;
};

} // namespace libtokamap
