#pragma once

#include <nlohmann/json.hpp>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/typed_data_array.hpp"

namespace libtokamap
{

enum class InterpType : short { UNKNOWN, LINEAR /*CUBIC, SPLINE, NEAREST*/ };

NLOHMANN_JSON_SERIALIZE_ENUM(InterpType, {{InterpType::UNKNOWN, ""}, // will default to this on no match
                                          {InterpType::LINEAR, "LINEAR"}})

class InterpMapping : public Mapping
{
  public:
    InterpMapping() = delete;
    InterpMapping(std::string input, std::string base, std::string target, InterpType interp_type)
        : m_input{std::move(input)}, m_base{std::move(base)}, m_target{std::move(target)}, m_interp_type{interp_type} {};

    [[nodiscard]] TypedDataArray map(const MapArguments& arguments) const override;

  private:
    std::string m_input;
    std::string m_base;
    std::string m_target;
    InterpType m_interp_type;

    [[nodiscard]] TypedDataArray map_interp_args(const MapArguments& arguments, const std::string& name) const;
};

} // namespace libtokamap
