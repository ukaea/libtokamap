#pragma once

#include <cstddef>
#include <cstdint>
#include <exprtk/exprtk.hpp>
#include <inja/inja.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/render.hpp"
#include "utils/typed_data_array.hpp"

namespace libtokamap
{

/**
 * @class ExprMapping
 * @brief ExprMapping class to the hold the EXPR MAP_TYPE after parsing from the
 * JSON mapping file
 *
 * The class holds an expression std::string 'm_expr' for evaluation and
 * computation when mapping. The string is evaluated by the templating library
 * pantor/inja on object creation. 'm_parameters' holds the std::strings of the
 * variables needed to evaluate the expression, eg. X+Y requires knowledge or
 * data retrieval of X and Y.
 *
 * The evaluation and computation of the expression is done using the expression
 * toolkit library exprtk, this is done in the templated function 'eval_expr'.
 * The needed parameters are required to be entries in the JSON mapping file for
 * the current group. Retrieval of the data is done as if the mapping was being
 * retrieved regardless of the expression operation.
 *
 */
class ExprMapping : public Mapping
{
  public:
    ExprMapping() = delete;
    ExprMapping(std::string expr, std::unordered_map<std::string, std::string> parameters)
        : m_expr{std::move(expr)}, m_parameters{std::move(parameters)} {};

    [[nodiscard]] TypedDataArray map(const MapArguments& arguments) const override;

  private:
    std::string m_expr;
    std::unordered_map<std::string, std::string> m_parameters;

    template <typename T> [[nodiscard]] TypedDataArray eval_expr(const MapArguments& arguments) const;
};

/**
 * @brief Function to perform the evaulation and computation of the expression string
 * using the exprtk library.
 *
 * @tparam T expression parameters template type, in theory the expression can
 * be evaluated with any floating-point type. However this is currently
 * hard-coded to use double.
 */
template <typename T> TypedDataArray ExprMapping::eval_expr(const MapArguments& arguments) const
{
    exprtk::symbol_table<T> symbol_table;
    exprtk::expression<T> expression;
    exprtk::parser<T> parser;

    bool vector_expr{false};
    bool first_vec_param{true};
    size_t result_size{1};

    std::vector<nlohmann::json> parameter_traces;
    std::vector<TypedDataArray> parameters;

    symbol_table.add_constants();
    for (const auto& [key, json_name] : m_parameters) {
        auto array = arguments.entries.at(json_name)->map(arguments);
        if (array.empty()) {
            return array;
        }

        if (arguments.trace_enabled) {
            parameter_traces.push_back({{key, array.trace()}});
        }

        if (array.data_type() != data_type_of<T>()) {
            if (array.data_type() == DataType::Float) {
                array = array.template convert<T, float>();
            } else if (array.data_type() == DataType::Int32) {
                array = array.template convert<T, int32_t>();
            } else {
                throw TokaMapError{"Unsupported type for parameter '" + key + "'"};
            }
        }

        T* raw_data = std::bit_cast<T*>(array.buffer());
        size_t data_size = array.size();

        if (data_size > 1) {
            symbol_table.add_vector(key, raw_data, data_size);
            if (first_vec_param) {
                // use size of first vector parameter to define the size
                // should use maximum but assumes all vectors in calculation same size
                result_size = array.size();
                first_vec_param = false;
            }
            vector_expr = true;
        } else {
            symbol_table.add_variable(key, *raw_data);
        }

        parameters.push_back(std::move(array));
    }

    std::vector<T> result(result_size);
    if (vector_expr) {
        symbol_table.add_vector("RESULT", result);
    } else {
        symbol_table.add_variable("RESULT", result.front());
    }
    expression.register_symbol_table(symbol_table);

    // replace patterns in expression if necessary, e.g. expression: RESULT:=X+Y
    std::string expr_string{"RESULT:=" + libtokamap::render(m_expr, arguments.global_data)};
    bool res = parser.compile(expr_string, expression);
    if (!res) {
        throw TokaMapError{"Failed to compile expression"};
    }

    // expression.value() executes the calculation and returns a value
    // however, result added to the expression to handle vector operations
    expression.value();

    TypedDataArray output;
    if (vector_expr) {
        output = TypedDataArray{result};
    } else {
        output = TypedDataArray{result.front()};
    }

    if (arguments.trace_enabled) {
        output.set_trace({{"map_type", "expression"}, {"expression", m_expr}, {"parameters", parameter_traces}});
    }

    return output;
}

} // namespace libtokamap
