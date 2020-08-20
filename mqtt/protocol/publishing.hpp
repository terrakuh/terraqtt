#ifndef MQTT_PROTOCOL_PUBLISHING_HPP_
#define MQTT_PROTOCOL_PUBLISHING_HPP_

#include "general.hpp"

namespace mqtt {
namespace protocol {

struct publish_header
{
	bool duplicate;
	bool retain;
	enum qos qos;
	std::uint16_t packet_identifier;
	string_type topic;
	blob_type payload;
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

inline void write_packet(byte_ostream& output, const publish_header& header)
{
	write_elements(
	    output,
	    static_cast<byte>(static_cast<int>(control_packet_type::publish) << 4 |
	                      (header.duplicate << 3 | static_cast<int>(header.qos) << 1 | header.retain)),
	    static_cast<variable_integer>(2 + header.topic.second + (header.qos != qos::at_most_once ? 2 : 0) +
	                                  header.payload.second));
	write(output, header.topic);

	if (header.qos != qos::at_most_once) {
		write_elements(output, header.packet_identifier);
	}

	write(output, header.payload);
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
