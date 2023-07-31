#pragma once

#include "context.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace terraqtt::protocol {

template<typename... Types>
[[nodiscard]] constexpr std::size_t required_packet_size() noexcept
{
	return (... + ([]() {
		        using type = std::remove_cv_t<std::remove_reference_t<Types>>;
		        if constexpr (std::is_integral_v<type> || std::is_enum_v<type>) {
			        return sizeof(type);
		        } else {
			        ([]<bool Flag = false>() { static_assert(Flag, "Unsupported type"); })();
		        }
	        })());
}

template<typename... Types>
[[nodiscard]] constexpr std::size_t required_packet_size(Types&&...) noexcept
{
	return required_packet_size<Types...>();
}

template<typename Type>
  requires(std::is_trivially_copyable_v<Type>)
inline const std::byte* read_field(const std::byte* source, Type& destination) noexcept
{
	if constexpr (std::endian::native == std::endian::little && sizeof(Type) > 1) {
		std::reverse_copy(source, source + sizeof(Type), reinterpret_cast<std::byte*>(&destination));
	} else {
		std::memcpy(&destination, source, sizeof(Type));
	}
	return source + sizeof(Type);
}

template<AsyncContext Context, typename... Types>
  requires((... && !std::is_const_v<Types>) )
inline typename Context::template return_type<std::size_t> read_fields(Context& context, Types&... fields)
{
	std::byte buffer[required_packet_size<Types...>()];

	std::error_code ec{};
	std::size_t read = 0;

	std::tie(ec, read) = co_await context.read(buffer);

	if (!ec) {
		auto ptr = buffer;
		(..., ptr = read_field(ptr, fields));
	}

	co_return context.return_value(ec, read);
}

} // namespace terraqtt::protocol
