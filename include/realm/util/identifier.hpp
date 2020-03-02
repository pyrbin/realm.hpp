#pragma once

#include <realm/core/types.hpp>

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
/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {
using hash_t = u64;

const hash_t fnv_basis = 14695981039346656037ull;
const hash_t fnv_prime = 1099511628211ull;

/**
 * @brief FNV1A Hash function
 * Simple hash function. Implementation taken from
 * https://notes.underscorediscovery.com/constexpr-fnv1a/
 * @param str
 * @param value
 * @return
 */
constexpr hash_t hash_fnv1a(const char* const str, const hash_t value = fnv_basis) noexcept
{
    return (str[0] == '\0') ? value : hash_fnv1a(&str[1], (value ^ hash_t(str[0])) * fnv_prime);
}

/**
 * @brief Identifier
 * Provides a constexpr unique mask/hash for a type.
 * Uses __PRETTY_FUNCTION__ macro & an implementation of FNV1A hashing.
 * @tparam T
 */
template<typename T>
struct identifier
{
private:
    static constexpr auto get() noexcept { return hash_fnv1a(__VALID_PRETTY_FUNC__); }

public:
    using value_type = hash_t;
    static constexpr value_type hash{ get() };
    static constexpr value_type mask{ static_cast<u64>(0x1L) << static_cast<u64>((hash) % 63L) };
};

template<typename T>
inline constexpr hash_t identifier_hash_v = internal::identifier<T>::hash;

template<typename T>
inline constexpr hash_t identifier_mask_v = internal::identifier<T>::mask;
} // namespace internal
} // namespace realm