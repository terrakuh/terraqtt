#pragma once

#include "../../error.hpp"
#include "../context.hpp"

#include <concepts>
#include <cstdint>
#include <string_view>

namespace terraqtt::protocol::v5 {

template<typename Type>
concept ReadableString = requires(Type&& string) {
	{
		string.size()
	} -> std::same_as<std::size_t>;
	{
		string.data()
	} -> std::convertible_to<const char*>;
};

template<typename Type>
concept String = requires(Type&& string) { string.max_size(); };

template<AsyncContext Context, String StringContainer>
inline typename Context::template return_type<StringContainer>
  read_string(Context& context, std::span<const std::byte> already_read_buffer = {})
{
	std::error_code ec{};
	StringContainer string{};

	co_return context.return_value(ec, std::move(string));
}

enum class QualityOfService : std::uint8_t {
	at_most_once,
	at_least_once,
	exactly_once,
};

enum class VariableInteger : std::uint32_t {};

template<AsyncContext Context>
inline typename Context::template return_type<VariableInteger> read_variable_integer(Context& context)
{
	std::error_code ec{};
	std::size_t read = 0;

	std::underlying_type_t<VariableInteger> result{};

	std::byte buffer[0];
	int multiplier = 1;
	do {
		std::tie(ec, read) = context.read(buffer);
		result += (static_cast<std::uint8_t>(*buffer) & 127) * multiplier;
		if (multiplier > 128 * 128 * 128) {
			ec = Error::malformed_variable_integer;
			co_return context.template return_value<VariableInteger>(ec);
		}
		multiplier *= 128;
	} while (static_cast<std::uint8_t>(*buffer) & 128);

	co_return context.return_value(ec, static_cast<VariableInteger>(result));
}


} // namespace terraqtt::protocol::v5
