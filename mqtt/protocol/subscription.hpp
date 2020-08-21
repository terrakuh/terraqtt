#ifndef MQTT_PROTOCOL_SUBSCRIPTION_HPP_
#define MQTT_PROTOCOL_SUBSCRIPTION_HPP_

#include "general.hpp"
#include "reader.hpp"
#include "writer.hpp"

namespace mqtt {
namespace protocol {

/**
 * @tparam String see write_blob()
 * @tparam TopicContainer must hold the type subscribe_header::topic
 */
template<typename String, typename TopicContainer>
struct subscribe_header
{
	struct topic
	{
		String filter;
		enum qos qos;
	};

	TopicContainer topics;
	std::uint16_t packet_identifier;
};

/**
 * @tparam ReturnCodeContainer must hold the type suback_header::return_code
 */
template<typename ReturnCodeContainer>
struct suback_header
{
	enum class return_code
	{
		success0 = 0x00,
		success1 = 0x01,
		success2 = 0x02,
		failure  = 0x80
	};

	static_assert(std::is_same<typename ReturnCodeContainer::value_type, return_code>::value,
	              "ReturnCodeContainer must return return_code");

	ReturnCodeContainer return_codes;
	std::uint16_t packet_identifier;
};

/**
 * @tparam TopicContainer must hold Strings
 */
template<typename TopicContainer>
struct unsubscribe_header
{
	TopicContainer topics;
	std::uint16_t packet_identifier;
};

struct unsuback_header
{
	std::uint16_t packet_identifier;
};

/**
 * Writes a subscribe packet to the output stream.
 *
 * @exception see write_blob(), write_elements()
 * @param output[in] the output stream
 * @param header the subscribe header
 * @tparam String see subscribe_header
 * @tparam TopicContainer must meet the requirements of *Container*
 */
template<typename String, typename TopicContainer>
inline void write_packet(byte_ostream& output, const subscribe_header<String, TopicContainer>& header)
{
	typename std::underlying_type<variable_integer>::type remaining = 2;

	for (const auto& i : header.topics) {
		if (!protected_add(remaining, 3u) || !protected_add(remaining, i.filter.size())) {
			throw protocol_error{ "remaining size cannot be represented" };
		}
	}

	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::subscribe) << 4 | 0x02),
	               static_cast<variable_integer>(remaining), header.packet_identifier);

	for (const auto& i : header.topics) {
		write_blob<true>(output, i.filter);
		write_elements(output, static_cast<byte>(i.qos));
	}
}

/**
 * Writes a suback packet to the output stream.
 *
 * @exception see write_elements()
 * @param output[in] the output stream
 * @param header the suback header
 * @tparam ReturnCodeContainer must meet the requirements of *Container*
 */
template<typename ReturnCodeContainer>
inline void write_packet(byte_ostream& output, const suback_header<ReturnCodeContainer>& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::suback) << 4),
	               static_cast<variable_integer>(2 + header.return_codes.size()));

	for (auto i : header.return_codes) {
		write_elements(output, static_cast<byte>(i));
	}
}

/**
 * Writes a unsubscribe packet to the output stream.
 *
 * @exception see write_blob(), write_elements()
 * @param output[in] the output stream
 * @param header the unsubscribe header
 * @tparam TopicContainer must meet the requirements of *Container*
 */
template<typename TopicContainer>
inline void write_packet(byte_ostream& output, const unsubscribe_header<TopicContainer>& header)
{
	typename std::underlying_type<variable_integer>::type remaining = 2;

	for (const auto& i : header.topics) {
		if (!protected_add(remaining, 2u) || !protected_add(remaining, i.size())) {
			throw protocol_error{ "remaining size cannot be represented" };
		}
	}

	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::unsubscribe) << 4 | 0x02),
	               static_cast<variable_integer>(remaining), header.packet_identifier);

	for (const auto& i : header.topics) {
		write_blob<true>(output, i);
	}
}

/**
 * Writes a unsuback packet to the output stream.
 *
 * @exception see write_elements()
 * @param output[in] the output stream
 * @param header the unsuback header
 */
inline void write_packet(byte_ostream& output, const unsuback_header& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::unsuback) << 4),
	               static_cast<variable_integer>(2), header.packet_identifier);
}

inline void read_packet(byte_istream& input, unsuback_header& header)
{
	byte type;

	read_element(input, type);

	if (type != static_cast<byte>(static_cast<int>(control_packet_type::unsuback) << 4)) {
		throw protocol_error{ "invalid unsuback flags" };
	}

	variable_integer remaining;

	read_element(input, remaining);

	if (remaining != static_cast<variable_integer>(2)) {
		throw protocol_error{ "invalid payload for unsuback" };
	}

	read_element(input, header.packet_identifier);
}

} // namespace protocol
} // namespace mqtt

#endif
