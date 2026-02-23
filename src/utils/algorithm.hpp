#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>
#include <iterator>
#include <numeric>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>

#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
// clang likes to pretend it is GCC so double check
#  define LIBTOKAMAP_IS_GCC __GNUC__ // probably
#endif

namespace libtokamap
{

inline void to_lower(std::string& string)
{
    std::ranges::transform(string, string.begin(), [](char c) { return static_cast<char>(::tolower(c)); });
}
inline void to_upper(std::string& string)
{
    std::ranges::transform(string, string.begin(), [](char c) { return static_cast<char>(::toupper(c)); });
}

inline std::string to_lower_copy(std::string_view string)
{
    std::string result(string);
    std::ranges::transform(result, result.begin(), [](char c) { return static_cast<char>(::tolower(c)); });
    return result;
}

inline std::string to_upper_copy(std::string_view string)
{
    std::string result(string);
    std::ranges::transform(result, result.begin(), [](char c) { return static_cast<char>(::toupper(c)); });
    return result;
}

template <typename T>
concept StringCollection = std::ranges::random_access_range<T> && std::is_same_v<typename T::value_type, std::string>;

template <typename T>
concept StringViewCollection =
    std::ranges::random_access_range<T> && std::is_same_v<typename T::value_type, std::string_view>;

#if !defined(LIBTOKAMAP_IS_GCC) || __GNUC__ >= 12

template <StringCollection Coll>
inline void split(Coll& collection, const std::string& input, const std::string& delimiter)
{
    for (const auto& token : std::views::split(input, delimiter)) {
        collection.emplace_back(&*token.begin(), token.size());
    }
}

template <StringViewCollection Coll>
inline void split(Coll& collection, const std::string& input, const std::string& delimiter)
{
    for (const auto& token : std::views::split(input, delimiter)) {
        collection.emplace_back(&*token.begin(), token.size());
    }
}

#else // !defined(LIBTOKAMAP_IS_GCC) || __GNUC__ >= 12

// std::views::split is broken for GCC 11

template <StringCollection Coll>
void split(Coll& collection, const std::string& input, const std::string& delimiter)
{
    if (delimiter.empty()) {
        collection.push_back(input);
        return;
    }

    std::string::size_type start = 0;
    while (true) {
        auto pos = input.find(delimiter, start);

        if (pos == std::string::npos) {
            collection.push_back(input.substr(start));
            break;
        }

        collection.push_back(input.substr(start, pos - start));
        start = pos + delimiter.size();
    }
}

template <StringViewCollection Coll>
void split(Coll& collection, const std::string& input, const std::string& delimiter)
{
    if (delimiter.empty()) {
        collection.push_back(input.data());
        return;
    }

    std::string::size_type start = 0;
    while (true) {
        auto pos = input.find(delimiter, start);

        if (pos == std::string::npos) {
            collection.emplace_back(&input[start]);
            break;
        }

        collection.emplace_back(&input[start], &input[pos]);
        start = pos + delimiter.size();
    }
}

#endif // !defined(LIBTOKAMAP_IS_GCC) || __GNUC__ >= 12

template <StringCollection Coll> inline std::string join(Coll& collection, const std::string& delimiter)
{
    if (collection.empty()) {
        return std::string{};
    }
    return std::accumulate(
        std::next(collection.begin()), collection.end(), collection[0],
        [&delimiter](const std::string& lhs, const std::string& rhs) { return lhs + delimiter + rhs; });
}

template <typename T>
concept NumericCollection = std::ranges::forward_range<T> && std::is_arithmetic_v<typename T::value_type>;

template <NumericCollection Coll> inline void iota(Coll& collection, typename Coll::value_type initial)
{
    std::iota(collection.begin(), collection.end(), initial);
}

inline std::string replace_last_copy(const std::string& input, const std::string& search,
                                     const std::string& replace_with)
{
    if (search.empty()) {
        return input; // Avoid infinite loops
    }

    std::string result = input;
    std::size_t pos = result.rfind(search);
    if (pos != std::string::npos) {
        result.replace(pos, search.length(), replace_with);
    }
    return result;
}

template <NumericCollection Coll> void transform_inplace(Coll& collection, std::function<float(float)> func)
{
    std::ranges::copy(std::views::transform(collection, func), collection.begin());
}

template <NumericCollection Coll> void transform(Coll& collection, Coll& out, std::function<float(float)> func)
{
    std::ranges::copy(std::views::transform(collection, func), std::back_inserter(out));
}

} // namespace libtokamap
