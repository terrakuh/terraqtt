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

inline void write_utf8(byte_ostream& output, const std::string& string)
{
	output.write(reinterpret_cast<const byte_ostream::char_type*>(string.c_str()), string.length());
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
inline void write_fixed_header(byte_ostream& output, std::uint32_t remaining_length)
{
	// todo: remaining length
	write_bytes(output, static_cast<int>(ControlPacketType) << 4);
}

template<control_packet_type ControlPacketType, typename... Args>
void write(byte_ostream& output, Args... args);

} // namespace detail
} // namespace mqtt

#endif
