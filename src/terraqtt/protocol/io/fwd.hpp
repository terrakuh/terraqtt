#pragma once

#include "../../config.hpp"

#include <bit>
#include <concepts>
#include <cstddef>
#include <span>
#include <system_error>
#include <type_traits>
#include <utility>

namespace terraqtt::protocol::io {

template<typename Type, int Version, typename = void>
struct IoContext;

using WriteSizeReturn = std::pair<std::error_code, std::size_t>;

struct OperationReturn {
	std::error_code ec;
	std::size_t consumed;
	/// How many bytes are at least required for the next read operation.
	std::size_t next_call_size;
};

template<typename Type, int Version>
concept Writeable = requires(Type&& value) {
	requires std::is_same_v<std::decay_t<decltype(IoContext<Type, Version>::min_size)>, std::size_t>;
	requires std::is_same_v<std::decay_t<decltype(IoContext<Type, Version>::max_request)>, std::size_t>;
	requires IoContext<Type, Version>::min_size <= TERRAQTT_BUFFER_SIZE;
	requires IoContext<Type, Version>::max_request <= TERRAQTT_BUFFER_SIZE;

	{
		IoContext<Type, Version>::write_size(value)
	} -> std::same_as<WriteSizeReturn>;

	/// Create an write context which will be passed by reference to write(). A return type of `void` is not
	/// allowed.
	IoContext<Type, Version>::make_write_context();

	{
		IoContext<Type, Version>::write(std::declval<decltype(IoContext<Type, Version>::make_write_context())&>(),
		                                std::declval<std::span<std::byte>>(), value)
	} -> std::same_as<OperationReturn>;
};

template<typename Type, int Version>
concept Readable = requires(Type&& value) {
	requires std::is_same_v<std::decay_t<decltype(IoContext<Type, Version>::min_size)>, std::size_t>;
	requires std::is_same_v<std::decay_t<decltype(IoContext<Type, Version>::max_request)>, std::size_t>;
	requires IoContext<Type, Version>::min_size <= TERRAQTT_BUFFER_SIZE;
	requires IoContext<Type, Version>::max_request <= TERRAQTT_BUFFER_SIZE;

	IoContext<Type, Version>::make_read_context();

	{
		/**
		 * Reads `Type` from the input `buffer` into the `value` field.
		 *
		 * @note Signature: `ReadReturn read(<unspecified>& context, std::span<const std::byte> buffer, Type&
		 * value)`
		 *
		 * @param context The context acquired from `make_read_context()`. This context is passed by reference and
		 * preserved over `read()` calls for the same `value`.
		 * @param buffer The input buffer sequence. For the first call this is at least `min_size` large. For any
		 * subsequent call at least `std::min(ReadReturn::next_read_size, max_request)`.
		 * @param value The output value. The value will be default constructed and preserved over multiple calls
		 * like `context`.
		 */
		IoContext<Type, Version>::read(std::declval<decltype(IoContext<Type, Version>::make_read_context())&>(),
		                               std::declval<std::span<const std::byte>>(), std::declval<Type&>())
	} -> std::same_as<OperationReturn>;
};

} // namespace terraqtt::protocol::io
