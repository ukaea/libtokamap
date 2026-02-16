#include "indices.hpp"

#include <utility>
#include <vector>
#include <deque>
#include <string>
#include <string_view>
#include <ctre/ctre.hpp>

static constexpr auto indices_re = ctll::fixed_string{ R"(\[(\d+)\])" };

/**
 * @brief Extract current indices and path tokens from the mapping path
 *
 * @param path_tokens deque of strings containing split mapping path tokens
 * @return {indices, processed_tokens} pair of the indices vector and tokens
 */
std::pair<std::vector<int>, std::deque<std::string>>
libtokamap::extract_indices(const std::deque<std::string_view>& path_tokens)
{
    std::vector<int> indices;
    std::deque<std::string> processed_tokens;

    for (const auto& token : path_tokens) {
        std::string result;
        auto last_pos = token.begin();

        for (auto match : ctre::search_all<indices_re>(token)) {
            const auto start = match.get<0>().begin();
            const auto end = match.get<0>().end();

            auto num = match.get<1>().to_string();
            indices.push_back(std::stoi(num));

            result.append(token.substr(last_pos - token.begin(), start - last_pos));
            result.append("[#]");

            last_pos = end;
        }

        result.append(token.substr(last_pos - token.begin(), token.end() - last_pos));
        processed_tokens.push_back(result);
        result.clear();
    }

    return {indices, processed_tokens};
}
