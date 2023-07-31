#pragma once

#include <cstddef>
#include <span>
#include <system_error>
#include <tuple>
#include <utility>

namespace terraqtt::protocol {

template<typename Type>
concept AsyncContext = requires(Type& context) {
	context.read_some(std::declval<std::span<std::byte>>());
	context.read(std::declval<std::span<std::byte>>());
	context.write(std::declval<std::span<const std::byte>>());
	context.template return_value<std::size_t>(std::error_code{});
	context.return_value(std::error_code{}, std::size_t{ 0 });

	[]() -> typename Type::template return_type<std::size_t> {
		std::byte buffer[1];
		std::error_code ec{};
		std::size_t size = 0;
		auto& context    = *reinterpret_cast<Type*>(0);

		// Reads.
		std::tie(ec, size) = co_await context.read_some(buffer);
		std::tie(ec, size) = co_await context.read(buffer);

		// Writes.
		std::tie(ec, size) = co_await context.write(buffer);

		// Return statements.
		co_return context.template return_value<std::size_t>(ec);
		co_return context.return_value(ec, size);
	};
};

}
