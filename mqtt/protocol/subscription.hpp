#ifndef MQTT_PROTOCOL_SUBSCRIPTION_HPP_
#define MQTT_PROTOCOL_SUBSCRIPTION_HPP_

#include "general.hpp"

namespace mqtt {
namespace protocol {

enum class suback_return_codes
{
	success0 = 0x00,
	success1 = 0x01,
	success2 = 0x02,
	failure  = 0x80
};

struct subscribe_header
{
	struct topic
	{
		string_type filter;
		enum qos qos;
	};

	std::uint16_t packet_identifier;
	std::pair<const topic*, std::size_t> topics;
};

inline void write_packet(byte_ostream& output, const subscribe_header& header)
{
	typename std::underlying_type<variable_integer>::type remaining = 2;

	for (auto i = header.topics.first, c = i + header.topics.second; i != c; ++i) {
		remaining += 2 + i->filter.second + 1;
	}

	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::subscribe) << 4 | 0x02),
	               static_cast<variable_integer>(remaining), header.packet_identifier);

	for (auto i = header.topics.first, c = i + header.topics.second; i != c; ++i) {
		write(output, i->filter);
		write_elements(output, static_cast<byte>(i->qos));
	}
}

} // namespace protocol
} // namespace mqtt

#endif
