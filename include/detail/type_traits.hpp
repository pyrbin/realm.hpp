
#include <tuple>
#include <type_traits>
#include <utility>

namespace realm {
namespace detail {

template<typename...>
inline constexpr bool is_unique = std::true_type{};

template<typename T, typename... Rest>
inline constexpr bool is_unique<T, Rest...> =
  std::bool_constant<(!std::is_same_v<T, Rest> && ...) && is_unique<Rest...>>{};

} // namespace detail
} // namespace realm