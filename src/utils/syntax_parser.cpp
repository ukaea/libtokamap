#include "syntax_parser.hpp"

#include <cstddef>
#include <ctre/ctre.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/fmt/fmt.h>
#include <stack>
#include <string>
#include <string_view>

using namespace std::string_literals;

namespace
{

constexpr auto indices_re = ctll::fixed_string{R"(\{\{(.*?)\}\})"};
constexpr auto simple_index_re = ctll::fixed_string{R"((.*)#(\d+)(.*))"};
constexpr auto array_index_re = ctll::fixed_string{R"(([^\[\]]*?)\[([^\[\]]*?)\](\.[^\[\]]*)?)"};

std::string expand_array(std::string input)
{
    while (const auto& match = ctre::search<array_index_re>(input)) {
        std::string_view name = match.get<1>();
        std::string_view index = match.get<2>();
        std::string_view post = match.get<3>();
        if (name.starts_with('(')) {
            name = name.substr(1, name.size() - 1);
        }
        if (post.ends_with(')')) {
            post = post.substr(0, post.size() - 1);
        }
        std::string result = fmt::format("at({}, {}){}", name, index, post);
        input = input.replace(match.begin() - input.begin(), match.size(), result);
    }
    return input;
}

std::string expand_indices(std::string input)
{
    while (const auto& match = ctre::search<simple_index_re>(input)) {
        std::string_view pre = match.get<1>();
        std::string_view index = match.get<2>();
        std::string_view post = match.get<3>();
        input = fmt::format("{}indices.{}{}", pre, index, post);
    }

    return expand_array(input);
}

void walk_json(nlohmann::json& root)
{
    std::stack<nlohmann::json*> stack;
    stack.push(&root);

    while (!stack.empty()) {
        nlohmann::json* current = stack.top();
        stack.pop();

        for (const auto& element : current->items()) {
            if (element.key() == "MAP_TYPE") {
                continue; // Skip MAP_TYPE elements
            }

            auto& node = element.value();
            if (node.is_string()) {
                node = libtokamap::process_string_node(node);
            } else if (node.is_object()) {
                stack.push(&node);
            }
        }
    }
}

std::string trim(const std::string& line)
{
    constexpr const char* white_space = " \t\v\r\n";
    std::size_t start = line.find_first_not_of(white_space);
    std::size_t end = line.find_last_not_of(white_space);
    return start == end ? std::string() : line.substr(start, end - start + 1);
}

} // namespace

std::string libtokamap::process_string_node(std::string value)
{
    std::string result;
    auto iter = value.begin();

    for (const auto& match : ctre::search_all<indices_re>(value)) {
        std::string prefix{iter, match.begin()};
        auto expression = match.get<1>().to_string();
        expression = trim(expression);
        result.append(prefix).append("{{ ").append(expand_indices(expression)).append(" }}");
        iter = match.end();
    }

    result.append(std::string{iter, value.end()});
    return result;
}

nlohmann::json libtokamap::expand_syntactic_sugar(nlohmann::json input)
{
    if (input.is_string()) {
        // parse forward mapping or simple string value
        std::string str = input;
        if (!str.empty() && str[0] == '@') {
            str = str.substr(1);
            input = {{"MAP_TYPE", "FORWARD"}, {"VALUE", str}};
        } else {
            input = {{"MAP_TYPE", "VALUE"}, {"VALUE", str}};
        }
    } else if (input.is_primitive()) {
        // parse simple non-string value
        input = {{"MAP_TYPE", "VALUE"}, {"VALUE", input}};
    }

    // walk object looking for strings with #N
    walk_json(input);

    return input;
}
