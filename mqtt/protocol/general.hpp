#ifndef MQTT_PROTOCOL_GENERAL_HPP_
#define MQTT_PROTOCOL_GENERAL_HPP_

#include <cstdint>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace mqtt {
namespace protocol {

typedef std::uint8_t byte;
typedef std::ostream byte_ostream;
typedef std::istream byte_istream;
typedef std::pair<const char*, std::uint16_t> string_type;
typedef std::pair<const byte*, std::size_t> blob_type;

enum class variable_integer : std::uint32_t
{
};

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

inline std::size_t size_of(byte_istream& input)
{
	const auto pos = input.tellg();

	input.seekg(0, std::ios::end);

	const auto end = input.tellg();

	input.seekg(pos);

	return static_cast<std::size_t>(end - pos);
}

template<typename Element>
constexpr std::size_t elements_max_size() noexcept
{
	return sizeof(typename std::decay<Element>::type);
}

template<typename Element, typename... Elements>
constexpr typename std::enable_if<(sizeof...(Elements) > 0), std::size_t>::type elements_max_size() noexcept
{
	return elements_max_size<Element>() + elements_max_size<Elements...>();
}

inline std::size_t write_element(byte* output, byte value) noexcept
{
	*output = value;

	return 1;
}

inline std::size_t write_element(byte* output, std::uint16_t value) noexcept
{
	output[0] = static_cast<byte>(value >> 8);
	output[1] = static_cast<byte>(value & 0xff);

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

template<typename Element>
inline byte* write_elements(byte* output, Element&& element)
{
	return output + write_element(output, std::forward<Element>(element));
}

template<typename Element, typename... Elements>
inline byte* write_elements(byte* output, Element&& element, Elements&&... elements)
{
	return write_elements(output + write_element(output, std::forward<Element>(element)),
	                      std::forward<Elements>(elements)...);
}

template<typename... Elements>
inline void write_elements(byte_ostream& output, Elements&&... elements)
{
	byte buffer[elements_max_size<Elements...>()];

	output.write(
	    reinterpret_cast<const byte_ostream::char_type*>(buffer),
	    static_cast<std::size_t>(write_elements(buffer, std::forward<Elements>(elements)...) - buffer));
}

inline void write(byte_ostream& output, blob_type blob)
{
	output.write(reinterpret_cast<const byte_ostream::char_type*>(blob.first), blob.second);
}

inline void write(byte_ostream& output, string_type string)
{
	write_elements(output, string.second);
	output.write(reinterpret_cast<const byte_ostream::char_type*>(string.first), string.second);
}

} // namespace protocol
} // namespace mqtt

#endif
