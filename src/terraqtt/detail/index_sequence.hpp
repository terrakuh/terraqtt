#pragma once

#include <utility>

namespace terraqtt::detail {

template<typename Type, Type Start, Type Count, Type... Ints>
struct MakeIntegerSequence;

template<typename Type, Type Start, Type... Ints>
struct MakeIntegerSequence<Type, Start, 0, Ints...> {
	using type = std::integer_sequence<Type, Ints...>;
};

template<typename Type, Type Start, Type Count, Type... Ints>
struct MakeIntegerSequence<Type, Start, Count, Ints...>
    : MakeIntegerSequence<Type, Start + 1, Count - 1, Ints..., Start> {};

template<std::size_t Start, std::size_t Count>
using make_index_sequence = typename MakeIntegerSequence<std::size_t, Start, Count>::type;

} // namespace terraqtt::detail
