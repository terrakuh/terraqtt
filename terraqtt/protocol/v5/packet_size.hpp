#pragma once

#include <cstddef>
#include <type_traits>

namespace terraqtt::protocol::v5 {

template<typename Type>
struct TypeIdentity {
	using type = Type;
};

[[nodiscard]] constexpr std::size_t required_packet_size(auto&&... fields) noexcept
{
	return (... + ([](auto identity) {
		        using type = std::remove_cv_t<std::remove_reference_t<decltype(identity)::type>>;
		        if constexpr (std::is_integral_v<type> || std::is_enum_v<type>) {
			        return sizeof(type);
		        } else if constexpr (std::is_array_v<type> && std::rank_v<type> == 1 &&
		                             std::extent_v<type> != 0) {
			        return std::extent_v<type>;
		        }
	        })(fields));
}

template<typename, typename... Types>
struct Const : std::false_type {};

template<typename... Types>
struct Const<std::void_t<int[required_packet_size(TypeIdentity<Types>{}...)]>, Types...>
    : std::true_type {};


} // namespace terraqtt::protocol::v5
