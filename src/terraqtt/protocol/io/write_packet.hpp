#pragma once

#include "context.hpp"
#include "general.hpp"

#include <cassert>

namespace terraqtt::protocol::io {

template<int Version, Writeable<Version>... Types>
inline std::pair<std::error_code, std::size_t> write_size(const Types&... fields)
{
	std::error_code ec{};
	std::size_t size = 0;
	const std::size_t sum =
	  (0 + ... + (ec ? 0 : (std::tie(ec, size) = IoContext<Types, Version>::write_size(fields), size)));
	return { ec, sum };
}

template<int Version, AsyncContext Context, Writeable<Version>... Types>
inline typename Context::template return_type<std::size_t> write_fields(Context& context,
                                                                        const Types&... fields)
{
	std::byte static_buffer[TERRAQTT_BUFFER_SIZE];
	// This is the area of `static_buffer` that can be written to.
	std::span<std::byte> buffer{ static_buffer };

	std::error_code ec{};
	std::size_t total_written = 0;

	((ec ? static_cast<void>(0)
	     : static_cast<void>(co_await [&](const auto& field) -> typename Context::template return_type<int> {
		       using IoContext_ = IoContext<std::remove_cvref_t<decltype(field)>, Version>;

		       auto write_context = IoContext_::make_write_context();
		       std::size_t required_size = IoContext_::min_size;

		       do {
			       if (buffer.size() < required_size) {
				       std::size_t written = 0;
				       std::tie(ec, written) = co_await context.write({ static_buffer, buffer.data() });
				       // TODO
				       //  assert((ec || written == sizeof(static_buffer) - buffer.size()));
				       buffer = { static_buffer };
				       total_written += written;

				       if (ec) {
					       break;
				       }
			       }

			       const auto r = IoContext_::write(write_context, buffer, field);
			       ec = r.ec;
			       required_size = std::min(r.next_call_size, IoContext_::max_request);
			       buffer = buffer.subspan(r.consumed);
		       } while (!ec && required_size != 0);

		       co_return context.return_value(ec, 0);
	       }(fields))),
	 ...);

	// Final flush.
	if (!ec && buffer.size() != sizeof(static_buffer)) {
		std::size_t written = 0;
		std::tie(ec, written) = co_await context.write({ static_buffer, buffer.data() });
		total_written += written;
	}

	co_return context.return_value(ec, total_written);
}

template<int Version, AsyncContext Context, Writeable<Version>... Types>
inline typename Context::template return_type<std::size_t>
  write_packet(Context& context, ControlPacketType type, std::uint8_t flags,
               std::uint32_t additional_payload_size, const Types&... fields)
{
	const auto [ec, size] = write_size<Version>(fields...);
	if (ec) {
		co_return context.return_value(ec, size);
	}

	const std::byte first = static_cast<std::byte>(static_cast<int>(type) << 4 | flags);
	co_return co_await write_fields<Version>(
	  context, first, static_cast<VariableInteger>(size + additional_payload_size), fields...);
}

template<typename... Types>
struct Properties {
	std::tuple<Types...> values;
};

template<typename... Types>
inline auto make_properties(Types&&... properties)
{
	return Properties{ std::make_tuple(std::forward<Types>(properties)...) };
}

template<int Version, AsyncContext Context, typename... PropertyTypes, typename... Types>
inline typename Context::template return_type<std::size_t>
  write_packet(Context& context, ControlPacketType type, std::uint8_t flags,
               std::uint32_t additional_payload_size, const Properties<PropertyTypes...>& properties,
               const Types&... fields)
{
	std::error_code ec{};
	std::size_t properties_size = 0;
	std::tie(ec, properties_size) = write_size<Version>(properties.values);
	if (ec) {
		co_return context.return_value(ec, std::size_t{ 0 });
	}

	std::size_t fields_size = 0;
	std::tie(ec, fields_size) = write_size<Version>(static_cast<VariableInteger>(properties_size), fields...);
	if (ec) {
		co_return context.return_value(ec, std::size_t{ 0 });
	}

	const std::byte first = static_cast<std::byte>(static_cast<int>(type) << 4 | flags);
	co_return co_await std::apply(
	  [&](const auto&... unpacked_properties) -> typename Context::template return_type<std::size_t> {
		  co_return co_await write_fields<Version>(
		    context, first, static_cast<VariableInteger>(fields_size + properties_size + additional_payload_size),
		    fields..., static_cast<VariableInteger>(properties_size), unpacked_properties...);
	  },
	  properties.values);
}

} // namespace terraqtt::protocol::io
