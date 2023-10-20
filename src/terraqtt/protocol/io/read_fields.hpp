#pragma once

#include "context.hpp"
#include "general.hpp"

#include <cassert>
#include <cstring>
#include <limits>
#include <tuple>

namespace terraqtt::protocol::io {

template<AsyncContext Context>
inline typename Context::template return_type<std::size_t>
  read_into_buffer(Context& context, std::span<std::byte> static_buffer, std::span<std::byte>& buffer,
                   std::size_t required_size, std::size_t max_size)
{
	if (max_size < required_size) {
		// TODO error
		co_return context.template return_value<std::size_t>(Error::bad_remaining_size, 0);
	}

	std::error_code ec{};
	std::size_t total_read = 0;

	if (buffer.size() < required_size) {
		if (static_buffer.size() < required_size) {
			// TODO error
		} else if (static_buffer.data() + static_buffer.size() - buffer.data() < required_size) {
			std::memmove(static_buffer.data(), buffer.data(), buffer.size());
			buffer = { static_buffer.data(), buffer.size() };
		}

		do {
			const std::size_t max_read =
			  std::min(static_cast<std::size_t>((static_buffer.data() + static_buffer.size()) -
			                                    (buffer.data() + buffer.size())),
			           max_size - buffer.size());
			std::size_t read = 0;
			std::tie(ec, read) = co_await context.read_some({ buffer.data() + buffer.size(), max_read });
			total_read += read;
			buffer = { buffer.data(), buffer.size() + read };
		} while (!ec && buffer.size() < required_size);
	}

	co_return context.return_value(ec, total_read);
}

/**
 * Reads from the given `context` into the `static_buffer` and parsing the `fields` from the `buffer`.
 *
 * @tparam Version The version of the MQTT protocol.
 * @tparam Conservative If `true`, reads only as little as possible. If `false`, `remaining_length` should be
 * set accordingly.
 *
 * @param context The asynchronous read context.
 * @param static_buffer The whole buffer that can be read to.
 * @param buffer The actual region of `static_buffer` that contains read data.
 * @param remaining_length The maximum amount of bytes that can be read. This includes what can be read from
 * `buffer`.
 * @param fields The output fields that need to be parsed.
 * @return The amount of read bytes from the context.
 */
template<int Version, bool Conservative, AsyncContext Context, Readable<Version>... Types>
  requires(sizeof...(Types) > 0)
inline typename Context::template return_type<std::size_t>
  read_fields(Context& context, std::span<std::byte> static_buffer, std::span<std::byte>& buffer,
              std::size_t remaining_length, Types&... fields)
{
	const std::size_t initial_buffer_size = buffer.size();
	std::error_code ec{};
	std::size_t total_read = 0;

	// Try to read the minimum size to reduce the amount of read calls.
	constexpr std::size_t min_size = (IoContext<Types, Version>::min_size + ...);
	if (min_size + buffer.size() <= remaining_length) {
		std::tie(ec, total_read) =
		  co_await read_into_buffer(context, static_buffer, buffer, min_size,
		                            Conservative ? min_size : std::numeric_limits<std::size_t>::max());
	}

	((ec ? static_cast<void>(0)
	     : static_cast<void>(co_await [&](auto& field) -> typename Context::template return_type<int> {
		       using IoContext_ = IoContext<std::remove_reference_t<decltype(field)>, Version>;
		       auto read_context = IoContext_::make_read_context();
		       std::size_t required_size = IoContext_::min_size;
		       do {
			       // Need to read more
			       if (buffer.size() < required_size) {
				       std::size_t read = 0;
				       std::tie(ec, read) = co_await read_into_buffer(
				         context, static_buffer, buffer, required_size,
				         Conservative ? remaining_length - total_read : std::numeric_limits<std::size_t>::max());
				       total_read += read;

				       if (ec) {
					       break;
				       }
			       }

			       const auto r = IoContext_::read(read_context, buffer, field);
			       ec = r.ec;
			       required_size = std::min(r.next_call_size, IoContext_::max_request);
			       buffer = buffer.subspan(r.consumed);
		       } while (!ec && required_size != 0);

		       co_return context.return_value(ec, 0);
	       }(fields))),
	 ...);

	co_return context.return_value(ec, total_read);
}

} // namespace terraqtt::protocol::io
