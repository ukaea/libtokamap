#include <libtokamap.hpp>

#include <cstddef>
#include <memory>
#include <span>

#include <utility>
#include <vector>

#include "utils/library_loader.hpp"

namespace
{

libtokamap::TypedDataArray dot_product(libtokamap::CustomMappingInputs& inputs,
                                       const libtokamap::CustomMappingParams& /*params*/)
{
    if (!inputs.contains("lhs")) {
        throw libtokamap::TokaMapError("dot_product expects input 'lhs'");
    }
    if (!inputs.contains("rhs")) {
        throw libtokamap::TokaMapError("dot_product expects input 'rhs'");
    }
    const auto& lhs = inputs["lhs"];
    const auto& rhs = inputs["rhs"];

    if (lhs.size() != rhs.size()) {
        throw libtokamap::TokaMapError("Vectors must have the same size");
    }
    if (lhs.rank() != 1 || rhs.rank() != 1) {
        throw libtokamap::TokaMapError("Vectors must be 1-dimensional");
    }
    if (lhs.data_type() != libtokamap::DataType::Float || rhs.data_type() != libtokamap::DataType::Float) {
        throw libtokamap::TokaMapError("Vectors must be of type float");
    }

    const auto lhs_data = lhs.data<float>();
    const auto rhs_data = rhs.data<float>();
    float result = 0.0F;

    for (size_t i = 0; i < lhs.size(); ++i) {
        result += lhs_data[i] * rhs_data[i];
    }

    return libtokamap::TypedDataArray{result};
}

} // namespace

extern "C" void LibTokaMapLibraryLoader(libtokamap::LibraryEntryInterface& interface)
{
    auto dot_product_wrapper = std::make_unique<libtokamap::LibraryFunctionPointer>(dot_product);
    interface.functions.emplace_back("custom", "dot_product", std::move(dot_product_wrapper));
}
