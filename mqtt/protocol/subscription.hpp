#ifndef MQTT_PROTOCOL_SUBSCRIPTION_HPP_
#define MQTT_PROTOCOL_SUBSCRIPTION_HPP_

#include "general.hpp"

#include <iterator>

namespace mqtt {
namespace protocol {

/**
 * @tparam ReturnCodeIterator must meet the requirements of LegacyInputIterator and must return
 * subscribe_header_in::topic
 */
template<typename TopicIterator>
struct subscribe_header_in
{
	struct topic
	{
		string_type filter;
		enum qos qos;
	};

	std::uint16_t packet_identifier;
	std::pair<TopicIterator, TopicIterator> topics;
};

template<typename TopicIterator>
struct subscribe_header_out
{
	struct topic
	{
		string_type filter;
		enum qos qos;
	};

	std::uint16_t packet_identifier;
	TopicIterator topics;
};

/**
 * @tparam ReturnCodeIterator must meet the requirements of LegacyInputIterator and must return
 * suback_header_out::return_code
 */
template<typename ReturnCodeIterator>
struct suback_header_out
{
	enum class return_code
	{
		success0 = 0x00,
		success1 = 0x01,
		success2 = 0x02,
		failure  = 0x80
	};

	std::uint16_t packet_identifier;
	std::pair<ReturnCodeIterator, ReturnCodeIterator> return_codes;
};

/**
 * @tparam ReturnCodeIterator must meet the requirements of LegacyInputIterator and must return
 * string_type
 */
template<typename TopicIterator>
struct unsubscribe_header_in
{
	std::uint16_t packet_identifier;
	std::pair<TopicIterator, TopicIterator> topics;
};

struct unsuback_header
{
	std::uint16_t packet_identifier;
};

template<typename TopicIterator>
inline void write_packet(byte_ostream& output, const subscribe_header_in<TopicIterator>& header)
{
	typename std::underlying_type<variable_integer>::type remaining = 2;

	for (auto i = header.topics.first; i != header.topics.second; ++i) {
		remaining += 2 + i->filter.second + 1;
	}

	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::subscribe) << 4 | 0x02),
	               static_cast<variable_integer>(remaining), header.packet_identifier);

	for (auto i = header.topics.first; i != header.topics.second; ++i) {
		write(output, i->filter);
		write_elements(output, static_cast<byte>(i->qos));
	}
}

template<typename ReturnCodeIterator>
inline void write_packet(byte_ostream& output, const suback_header_out<ReturnCodeIterator>& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::suback) << 4),
	               static_cast<variable_integer>(
	                   2 + std::distance(header.return_codes.first, header.return_codes.second)));

	for (auto i = header.topics.first; i != header.topics.second; ++i) {
		write_elements(output, static_cast<byte>(*i));
	}
}

template<typename TopicIterator>
inline void write_packet(byte_ostream& output, const unsubscribe_header_in<TopicIterator>& header)
{
	typename std::underlying_type<variable_integer>::type remaining = 2;

	for (auto i = header.topics.first; i != header.topics.second; ++i) {
		remaining += 2 + i->second;
	}

	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::unsubscribe) << 4 | 0x02),
	               static_cast<variable_integer>(remaining), header.packet_identifier);

	for (auto i = header.topics.first; i != header.topics.second; ++i) {
		write(output, *i);
	}
}

inline void write_packet(byte_ostream& output, const unsuback_header& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::unsuback) << 4),
	               static_cast<variable_integer>(2), header.packet_identifier);
}

} // namespace protocol
} // namespace mqtt

#endif
