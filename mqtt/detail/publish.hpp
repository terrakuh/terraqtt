#ifndef MQTT_DETAIL_PUBLISH_HPP_
#define MQTT_DETAIL_PUBLISH_HPP_

#include "general.hpp"

namespace mqtt {
namespace detail {

struct publish_header
{
	bool duplicate;
	bool retain;
	enum qos qos;
	std::uint16_t packet_identifier;
	string_type topic;
	blob_type payload;
};

inline void write_packet(byte_ostream& output, const publish_header& header)
{
	write_elements(
	    output,
	    static_cast<byte>(static_cast<int>(control_packet_type::publish) << 4 |
	                      (header.duplicate << 3 | static_cast<int>(header.qos) << 1 | header.retain)),
	    static_cast<variable_integer>(2 + header.topic.second + 0 + header.payload.second));

	write(output, header.topic);
	//write_elements(output, header.packet_identifier);
	write(output, header.payload);
}

} // namespace detail
} // namespace mqtt

#endif
