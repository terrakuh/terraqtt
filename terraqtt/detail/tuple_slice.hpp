#pragma once

#include "templated_of.hpp"

#include <tuple>
#include <utility>

namespace terraqtt::detail {

template<std::size_t Offset, std::size_t Count, TemplatedOf<std::tuple> Tuple, std::size_t... Indices>
  requires(Offset + Count <= std::tuple_size_v<std::decay_t<Tuple>>)
inline auto tuple_slice(Tuple&& tuple, std::index_sequence<Indices...>)
{
	return std::make_tuple(std::get<Offset + Indices>(std::forward<Tuple>(tuple))...);
}

template<std::size_t Offset, std::size_t Count, TemplatedOf<std::tuple> Tuple>
  requires(Offset + Count <= std::tuple_size_v<std::decay_t<Tuple>>)
inline auto tuple_slice(Tuple&& tuple)
{
	return tuple_slice<Offset, Count>(std::forward<Tuple>(tuple), std::make_index_sequence<Count>{});
}

template<std::size_t Offset, std::size_t Count, TemplatedOf<std::tuple> Tuple, std::size_t... Indices>
  requires(Offset + Count <= std::tuple_size_v<std::decay_t<Tuple>>)
inline auto forward_tuple_slice(Tuple&& tuple, std::index_sequence<Indices...>)
{
	return std::forward_as_tuple(std::get<Offset + Indices>(std::forward<Tuple>(tuple))...);
}

template<std::size_t Offset, std::size_t Count, TemplatedOf<std::tuple> Tuple>
  requires(Offset + Count <= std::tuple_size_v<std::decay_t<Tuple>>)
inline auto forward_tuple_slice(Tuple&& tuple)
{
	return forward_tuple_slice<Offset, Count>(std::forward<Tuple>(tuple), std::make_index_sequence<Count>{});
}

} // namespace terraqtt::detail
