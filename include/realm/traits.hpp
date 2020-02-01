
#include <tuple>
#include <type_traits>
#include <utility>

namespace detail {

template<typename...>
inline constexpr bool is_unique = std::true_type{};

template<typename T, typename... Rest>
inline constexpr bool is_unique<T, Rest...> =
  std::bool_constant<(!std::is_same_v<T, Rest> && ...) && is_unique<Rest...>>{};

template<class T>
struct is_viable_component : std::integral_constant<bool, std::is_class<T>::value>
{};

template<typename T>
using is_viable_component_v = typename detail::is_viable_component<T>::value;

template<typename... Ts>
using tuple_cat_t = decltype(std::tuple_cat(std::declval<Ts>()...));

template<typename T, typename... Ts>
using remove_in_tuple = tuple_cat_t<typename std::conditional<std::is_same<T, Ts>::value,
                                                              std::tuple<>,
                                                              std::tuple<Ts>>::type...>;

} // namespace detail
