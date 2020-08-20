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
	span<const char*> topic;
	span<const byte*> payload;
};

template<>
inline void write<control_packet_type::publish, const publish_header&>(byte_ostream& output,
                                                                       const publish_header& header)
{
	// write_fixed_header<control_packet_type::publish>(output, 0);
	write_bytes(output, static_cast<int>(control_packet_type::publish) << 4 |
	                        (header.duplicate << 3 | static_cast<int>(header.qos) << 1 | header.retain));

	write_utf8(output, header.topic);

	if (header.qos != qos::at_most_once) {
		write_bytes(output, to_byte_sequence(header.packet_identifier));
	}

	output.write(reinterpret_cast<const byte_ostream::char_type*>(header.payload.begin()),
	             header.payload.size());
}

} // namespace detail
} // namespace mqtt

#endif
