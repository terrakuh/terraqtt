#ifndef MQTT_PROTOCOL_PUBLISHING_HPP_
#define MQTT_PROTOCOL_PUBLISHING_HPP_

#include "general.hpp"
#include "reader.hpp"
#include "writer.hpp"

namespace mqtt {
namespace protocol {

template<typename String, typename Blob>
struct publish_header
{
	bool duplicate;
	bool retain;
	enum qos qos;
	std::uint16_t packet_identifier;
	String topic;
	Blob payload;
};

struct puback_header
{
	std::uint16_t packet_identifier;
};

struct pubrec_header
{
	std::uint16_t packet_identifier;
};

struct pubrel_header
{
	std::uint16_t packet_identifier;
};

template<typename String, typename Blob>
inline void write_packet(byte_ostream& output, const publish_header<String, Blob>& header)
{
	typename std::underlying_type<variable_integer>::type remaining =
	    2 + (header.qos != qos::at_most_once ? 2 : 0);

	if (!protected_add(remaining, header.topic.size()) || !protected_add(remaining, header.payload.size())) {
		throw protocol_error{ "remaining size cannot be represented" };
	}

	write_elements(
	    output,
	    static_cast<byte>(static_cast<int>(control_packet_type::publish) << 4 |
	                      (header.duplicate << 3 | static_cast<int>(header.qos) << 1 | header.retain)),
	    static_cast<variable_integer>(remaining));
	write_blob<true>(output, header.topic);

	if (header.qos != qos::at_most_once) {
		write_elements(output, header.packet_identifier);
	}

	write_blob<false>(output, header.payload);
}

inline void write_packet(byte_ostream& output, const puback_header& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::puback) << 4),
	               static_cast<variable_integer>(2), header.packet_identifier);
}

inline void write_packet(byte_ostream& output, const pubrec_header& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::pubrec) << 4),
	               static_cast<variable_integer>(2), header.packet_identifier);
}

inline void write_packet(byte_ostream& output, const pubrel_header& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::pubrel) << 4),
	               static_cast<variable_integer>(2), header.packet_identifier);
}

} // namespace protocol
} // namespace mqtt

#endif
