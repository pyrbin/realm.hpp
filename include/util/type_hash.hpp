#pragma once

#include <cstddef>

// TODO: implement a fallback id generation
#if defined(__GNUC__)
#define __VALID_PRETTY_FUNC__ __PRETTY_FUNCTION__
#elif defined(__clang__)
#define __VALID_PRETTY_FUNC__ __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define __VALID_PRETTY_FUNC__ __FUNCSIG__
#else
#error "Compiler not supported"
#endif

namespace realm {
namespace internal {
using hash_t = uint64_t;

inline const hash_t fnv_basis = 14695981039346656037ull;
inline const hash_t fnv_prime = 1099511628211ull;

/**
 * @brief https://notes.underscorediscovery.com/constexpr-fnv1a/
 * @param str
 * @param value
 * @return
 */
inline constexpr hash_t
hash_fnv1a(const char* const str, const hash_t value = fnv_basis) noexcept
{
    return (str[0] == '\0') ? value
                            : hash_fnv1a(&str[1], (value ^ hash_t(str[0])) * fnv_prime);
}

template<typename T>
struct type_hash
{
private:
    static constexpr auto gen_hash() noexcept
    {
        return internal::hash_fnv1a(__VALID_PRETTY_FUNC__);
    }

public:
    using value_type = internal::hash_t;
    static constexpr value_type value{ gen_hash() };
};
} // namespace internal
} // namespace realm