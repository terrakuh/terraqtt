#ifndef MQTT_PROTOCOL_CONNECTION_HPP_
#define MQTT_PROTOCOL_CONNECTION_HPP_

#include "general.hpp"
#include "reader.hpp"
#include "writer.hpp"

#include <limits>
#include <type_traits>
#include <utility>

namespace mqtt {
namespace protocol {

template<typename String, typename WillMessage, typename Password>
struct connect_header
{
	/** must be between 1-23 and may only contain
	 * "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" */
	String client_identifier;
	std::pair<String, WillMessage>* will;
	typename std::remove_reference<String>::type* username;
	typename std::remove_reference<Password>::type* password;
	qos will_qos;
	bool will_retain;
	bool clean_session;
	/** the keep alive timeout in seconds; 0 means no timeout */
	std::uint16_t keep_alive;
};

struct disconnect_header
{};

template<typename String, typename WillMessage, typename Password>
inline void write_packet(byte_ostream& output, const connect_header<String, WillMessage, Password>& header)
{
	if (!header.will && (header.will_qos != qos::at_most_once || header.will_retain)) {
		throw protocol_error{ "invalid will" };
	} else if (!header.username && header.password) {
		throw protocol_error{ "invalid username/password combination" };
	} else if (!header.clean_session && header.client_identifier.empty()) {
		throw protocol_error{ "empty client identifier" };
	}

	// fixed header & protocol name & level & connect flags
	typename std::underlying_type<variable_integer>::type remaining =
	    12 + (header.will ? 4 : 0) + (header.username ? 2 : 0) + (header.password ? 2 : 0);
	const auto protocol_level = byte{ 0x04 };
	const auto connect_flags  = static_cast<byte>(
        (header.username ? 0x80 : 0x00) | (header.password ? 0x40 : 0x00) | (header.will_retain << 5) |
        (static_cast<int>(header.will_qos) << 3) | (header.will ? 0x04 : 0x00) | (header.clean_session << 1));

	if (!protected_add(remaining, header.client_identifier.size()) ||
	    (header.will && !protected_add(remaining, header.will->first.size())) ||
	    (header.will && !protected_add(remaining, header.will->second.size())) ||
	    (header.username && !protected_add(remaining, header.username->size())) ||
	    (header.password && !protected_add(remaining, header.password->size()))) {
		throw protocol_error{ "remaining size cannot be represented" };
	}

	write_elements(output, byte{ static_cast<int>(control_packet_type::connect) << 4 },
	               static_cast<variable_integer>(remaining), std::uint16_t{ 4 }, byte{ 'M' }, byte{ 'Q' },
	               byte{ 'T' }, byte{ 'T' }, protocol_level, connect_flags, header.keep_alive);

	// payload
	write_blob<true>(output, header.client_identifier);

	if (header.will) {
		write_blob<true>(output, header.will->first);
		write_blob<true>(output, header.will->second);
	}

	if (header.username) {
		write_blob<true>(output, *header.username);
	}

	if (header.password) {
		write_blob<true>(output, *header.password);
	}
}

/**
 * Writes a disconnect packet to the output stream.
 *
 * @exception see write_elements()
 * @param output[in] the output stream
 * @param header the disconnect header
 */
inline void write_packet(byte_ostream& output, const disconnect_header& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::disconnect) << 4),
	               static_cast<variable_integer>(0));
}

} // namespace protocol
} // namespace mqtt

#endif
