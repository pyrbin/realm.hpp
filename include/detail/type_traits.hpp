
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

template<typename... Ts>
using tuple_cat_t = decltype(std::tuple_cat(std::declval<Ts>()...));

template<typename T, typename... Ts>
using remove_in_tuple = tuple_cat_t<typename std::conditional<std::is_same<T, Ts>::value,
                                                              std::tuple<>,
                                                              std::tuple<Ts>>::type...>;
} // namespace detail
} // namespace realm