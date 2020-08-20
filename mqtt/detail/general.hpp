#ifndef MQTT_DETAIL_GENERAL_HPP_
#define MQTT_DETAIL_GENERAL_HPP_

#include <cstdint>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>

namespace mqtt {
namespace detail {

typedef std::uint8_t byte;
typedef std::ostream byte_ostream;
typedef std::istream byte_istream;

static_assert(sizeof(byte) == sizeof(byte_ostream::char_type), "sizes mismatch");

template<typename Iterable>
class span
{
public:
	span(Iterable first, Iterable second) noexcept : _first{ std::move(first) }, _second{ std::move(second) }
	{}
	span(Iterable data, std::size_t size) noexcept : _first{ std::move(data) }, _second{ data + size }
	{}
	Iterable begin() const noexcept
	{
		return _first;
	}
	Iterable end() const noexcept
	{
		return _second;
	}
	std::size_t size() const noexcept
	{
		return static_cast<std::size_t>(_second - _first);
	}

private:
	Iterable _first;
	Iterable _second;
};

enum class control_packet_type
{
	connect = 1,
	connack,
	publish,
	puback,
	pubrec,
	pubrel,
	pubcomp,
	subscribe,
	suback,
	unsubscribe,
	unsuback,
	pingreq,
	pingresp,
	disconnect,
};

enum class qos
{
	at_most_once,
	at_least_once,
	exactly_once
};

enum class variable_integer : std::uint32_t
{
};

class protocol_error : public std::runtime_error
{
public:
	using runtime_error::runtime_error;
};

template<typename Type>
inline std::tuple<> to_byte_sequence(Type value)
{
	return {};
}

inline std::size_t size_of(byte_istream& input)
{
	const auto pos = input.tellg();

	input.seekg(0, std::ios::end);

	const auto end = input.tellg();

	input.seekg(pos);

	return static_cast<std::size_t>(end - pos);
}

inline std::size_t write_element(byte* output, byte value) noexcept
{
	*output = value;

	return 1;
}

inline std::size_t write_element(byte* output, std::uint16_t value) noexcept
{
	*output = static_cast<byte>(value >> 8);
	*output = static_cast<byte>(value & 0xff);

	return 2;
}

inline std::size_t write_element(byte* output, variable_integer value)
{
	const auto v = static_cast<typename std::underlying_type<variable_integer>::type>(value);

	if (v < 128) {
		*output = static_cast<byte>(v);

		return 1;
	} else if (v < 16384) {
		output[0] = static_cast<byte>(v >> 7 | 0x80);
		output[1] = static_cast<byte>(v & 0x7f);

		return 2;
	} else if (v < 2097152) {
		output[0] = static_cast<byte>(v >> 14 & 0x7f | 0x80);
		output[1] = static_cast<byte>(v >> 7 & 0x7f | 0x80);
		output[2] = static_cast<byte>(v & 0x7f);

		return 3;
	} else if (v < 268435456) {
		output[0] = static_cast<byte>(v >> 21 & 0x7f | 0x80);
		output[1] = static_cast<byte>(v >> 14 & 0x7f | 0x80);
		output[2] = static_cast<byte>(v >> 7 & 0x7f | 0x80);
		output[3] = static_cast<byte>(v & 0x7f);

		return 4;
	}

	throw protocol_error{ "variable size is too large" };
}

template<typename... Elements>
inline void write_elements(byte_ostream& output, Elements&&... elements)
{
	byte buffer[size_of_elements<Elements...>()];
}

inline void write_utf8(byte_ostream& output, span<const char*> string)
{
	output.write(reinterpret_cast<const byte_ostream::char_type*>(string.begin()), string.size());
}

template<typename... Bytes>
inline void write_bytes(byte_ostream& output, Bytes... bytes)
{
	byte buffer[] = { static_cast<byte>(bytes)... };

	output.write(reinterpret_cast<const byte_ostream::char_type*>(buffer), sizeof(buffer));
}

template<typename Tuple>
inline void write_byte_sequence(byte_ostream& output, Tuple&& tuple)
{}

template<control_packet_type ControlPacketType>
inline void write_fixed_header(byte_ostream& output, int flags, std::uint32_t remaining_length)
{
	// todo: remaining length
	write_bytes(output, static_cast<int>(ControlPacketType) << 4);
}

template<control_packet_type ControlPacketType, typename... Args>
void write(byte_ostream& output, Args... args);

} // namespace detail
} // namespace mqtt

#endif
