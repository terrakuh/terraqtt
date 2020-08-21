#ifndef MQTT_PROTOCOL_READER_HPP_
#define MQTT_PROTOCOL_READER_HPP_

#include "general.hpp"

namespace mqtt {
namespace protocol {
/*
inline control_packet_type peek_type(byte_istream& input)
{
	return input.good() ? static_cast<control_packet_type>(input.peek()) : throw io_error{ "failed to read" };
}

inline void read_element(byte_istream& input, byte& out)
{
	input.read(reinterpret_cast<byte_istream::char_type*>(&out), sizeof(out));

	if (!input) {
		throw io_error{ "failed to read" };
	}
}

inline void read_element(byte_istream& input, std::uint16_t& out)
{
	byte_istream::char_type buffer[2];

	input.read(buffer, sizeof(buffer));

	if (!input) {
		throw io_error{ "failed to read" };
	}

	out = buffer[0] << 8 | buffer[1];
}

inline void read_element(byte_istream& input, variable_integer& out)
{
	typename std::underlying_type<variable_integer>::type value = 0;

	for (auto i = 0; i < 4; ++i) {
		const auto c = input.get();

		if (!input) {
			throw io_error{ "failed to read" };
		}

		value = value << 7 | (c & 0x7f);

		if (!(c & 0x80)) {
			break;
		}
	}

	out = static_cast<variable_integer>(value);
}

template<typename Element>
inline byte* read_element(byte* output, Element&& element)
{
	return output + read_element(output, std::forward<Element>(element));
}

template<typename Element, typename... Elements>
inline byte* read_elements(byte* output, Element&& element, Elements&&... elements)
{
	return read_elements(output + read_element(output, std::forward<Element>(element)),
	                     std::forward<Elements>(elements)...);
}

template<typename... Elements>
inline void read_elements(byte_ostream& output, Elements&&... elements)
{
	byte buffer[elements_max_size<Elements...>()];

	output.write(
	    reinterpret_cast<const byte_ostream::char_type*>(buffer),
	    static_cast<std::size_t>(read_elements(buffer, std::forward<Elements>(elements)...) - buffer));
}

inline void read(byte_ostream& output, blob_type blob)
{
	output.write(reinterpret_cast<const byte_ostream::char_type*>(blob.first), blob.second);
}

inline void read(byte_ostream& output, string_type string)
{
	read_elements(output, string.second);
	output.write(reinterpret_cast<const byte_ostream::char_type*>(string.first), string.second);
}
*/
} // namespace protocol
} // namespace mqtt

#endif
