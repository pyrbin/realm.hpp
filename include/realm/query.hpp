#pragma once

#include "concept.hpp"

namespace realm {

struct query_t
{};

template<typename... T>
struct query : public query_t
{
    query() requires QueryPack<T...> {}
};

} // namespace realm
