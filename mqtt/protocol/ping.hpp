#ifndef MQTT_PROTOCOL_PING_HPP_
#define MQTT_PROTOCOL_PING_HPP_

#include "general.hpp"
#include "reader.hpp"
#include "writer.hpp"

namespace mqtt {
namespace protocol {

struct pingreq_header
{};

struct pingresp_header
{};

inline void write_packet(byte_ostream& output, const pingreq_header& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::pingreq) << 4),
	               static_cast<variable_integer>(0));
}

inline void read_packet(byte_istream& input, pingreq_header& header)
{
	byte type;

	read_element(input, type);

	if (type != static_cast<byte>(static_cast<int>(control_packet_type::pingreq) << 4)) {
		throw protocol_error{ "invalid pingreq flags" };
	}

	variable_integer remaining;

	read_element(input, remaining);

	if (remaining != static_cast<variable_integer>(0)) {
		throw protocol_error{ "no payload allow in pingreq" };
	}
}

inline void write_packet(byte_ostream& output, const pingresp_header& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::pingresp) << 4),
	               static_cast<variable_integer>(0));
}

inline void read_packet(byte_istream& input, pingresp_header& header)
{
	byte type;

	read_element(input, type);

	if (type != static_cast<byte>(static_cast<int>(control_packet_type::pingresp) << 4)) {
		throw protocol_error{ "invalid pingresp flags" };
	}

	variable_integer remaining;

	read_element(input, remaining);

	if (remaining != static_cast<variable_integer>(0)) {
		throw protocol_error{ "no payload allow in pingresp" };
	}
}

} // namespace protocol
} // namespace mqtt

#endif
