#pragma once

#include "read_fields.hpp"

namespace terraqtt::protocol::io {

template<int Version, AsyncContext Context, std::size_t BufferSize = TERRAQTT_BUFFER_SIZE>
class Reader {
public:
	Reader(Context& context) : _context{ context } {}

	/// Reads the fixed MQTT header. This must be called before any other method. Returns the amount of bytes
	/// read.
	typename Context::template return_type<std::size_t> read_fixed_header()
	{
		VariableInteger payload{};
		const auto [ec, read] = co_await io::read_fields<Version, Conservative>(_context, _static_buffer, _buffer,
		                                                                        5, _first_byte, payload);
		_payload_length = to_underlying(payload);
		_remaining_length = to_underlying(payload);
		co_return _context.return_value(ec, read);
	}
	Context& context() noexcept { return _context; }
	ControlPacketType packet_type() const noexcept { return static_cast<ControlPacketType>(_first_byte >> 4); }
	std::uint8_t control_packet_flags() const noexcept { return _first_byte & 0xf; }
	/// The total payload length of the MQTT packet including variable header.
	std::underlying_type_t<VariableInteger> payload_length() const noexcept { return _payload_length; }
	/// Returns the remaining length that has not beed read.
	std::underlying_type_t<VariableInteger> remaining_length() const noexcept
	{
		return _remaining_length + _buffer.size();
	}
	bool done() const noexcept { return remaining_length() == 0; }
	template<Readable<Version>... Types>
	typename Context::template return_type<std::size_t> read_fields(Types&... fields)
	{
		const auto [ec, read] = co_await io::read_fields<Version, Conservative>(_context, _static_buffer, _buffer,
		                                                                        _remaining_length, fields...);
		_remaining_length -= read;
		co_return _context.return_value(ec, read);
	}

private:
	Context& _context;
	std::uint8_t _first_byte = 0;
	std::underlying_type_t<VariableInteger> _payload_length{};
	std::underlying_type_t<VariableInteger> _remaining_length{};
	std::byte _static_buffer[BufferSize];
	std::span<std::byte> _buffer{ _static_buffer, 0 };
};

} // namespace terraqtt::protocol::io
