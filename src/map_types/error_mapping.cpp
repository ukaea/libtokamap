#include "map_types/error_mapping.hpp"

#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/compiler.hpp"
#include "utils/typed_data_array.hpp"

namespace
{

std::vector<double> to_doubles(const libtokamap::TypedDataArray& array, const std::string& name)
{
    if (array.data_type() == libtokamap::DataType::Float) {
        const auto* data = array.data<float>();
        return std::vector<double>{data, data + array.size()};
    }
    if (array.data_type() == libtokamap::DataType::Double) {
        return array.to_vector<double>();
    }
    throw libtokamap::DataTypeError{"ERROR reference '" + name + "' must be floating point, got " +
                                    libtokamap::data_type_name(array.data_type())};
}

double point_error(libtokamap::ErrorType error_type, double point, double absolute, double relative)
{
    switch (error_type) {
        case libtokamap::ErrorType::ABSOLUTE:
            return absolute;
        case libtokamap::ErrorType::RELATIVE:
            return relative * std::abs(point);
        case libtokamap::ErrorType::MAX_ABS_REL:
            return std::max(absolute, relative * std::abs(point)); // EFIT++-ism 
        // case libtokamap::ErrorType::QUADRATURE:
        //   could add in the future (absolute/abs(point) + relative)*abs(point)
        //   can't really think of a situation we would need this though
        case libtokamap::ErrorType::UNKNOWN:
            throw libtokamap::ProcessingError{"Unknown error type"};
    }
    LIBTOKAMAP_UNREACHABLE
}

} // namespace

libtokamap::TypedDataArray libtokamap::ErrorMapping::map(const libtokamap::MapArguments& arguments) const
{
    if (!arguments.entries.contains(m_reference)) {
        throw libtokamap::MappingError{"Mapping '" + m_reference + "' referenced by ERROR mapping not found"};
    }
    const auto reference_array = arguments.entries.at(m_reference)->map(arguments);

    if (reference_array.empty()) {
        throw libtokamap::ProcessingError{"ERROR reference '" + m_reference + "' is empty"};
    }

    const auto reference = to_doubles(reference_array, m_reference);

    const double absolute = m_absolute.value_or(0.0);
    const double relative = m_relative.value_or(0.0);

    std::vector<double> errors(reference.size());
    std::ranges::transform(reference, errors.begin(), [&](double point) {
        return point_error(m_error_type, point, absolute, relative);
    });

    // use float if originally float, and keep the reference shape
    // borrow from interp_mapping by AJP (written by AJP)
    if (reference_array.data_type() == libtokamap::DataType::Float) {
        std::vector<float> result(errors.begin(), errors.end());
        return libtokamap::TypedDataArray{result, reference_array.shape()};
    }
    return libtokamap::TypedDataArray{errors, reference_array.shape()};
}
