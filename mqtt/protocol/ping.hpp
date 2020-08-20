#ifndef MQTT_PROTOCOL_PING_HPP_
#define MQTT_PROTOCOL_PING_HPP_

#include "general.hpp"

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

inline void write_packet(byte_ostream& output, const pingresp_header& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::pingresp) << 4),
	               static_cast<variable_integer>(0));
}

} // namespace protocol
} // namespace mqtt

#endif
