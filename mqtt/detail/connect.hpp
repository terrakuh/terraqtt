#ifndef MQTT_DETAIL_CONNECT_HPP_
#define MQTT_DETAIL_CONNECT_HPP_

#include "general.hpp"

#include <limits>
#include <utility>

namespace mqtt {
namespace detail {

struct connect_header
{
	// typedef std::pair<string_type, byte_istream&> will_type;

	enum qos qos;
	bool will_retain;
	bool clean_session;
	/** the keep alive timeout in seconds; 0 means no timeout */
	std::uint16_t keep_alive;
	/** must be between 1-23 and may only contain
	 * "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" */
	string_type client_identifier;
	//	std::optional<will_type> will;
	//	std::optional<string_type> username;
	//	std::optional<byte_istream&> password;
};

inline bool check_client_identifier(const string_type& identifier)
{
	for (auto i = identifier.first, c = i + identifier.second; i != c; ++i) {
		if (!std::isalnum(*i)) {
			return false;
		}
	}

	return identifier.second;
}

inline void write_packet(byte_ostream& output, const connect_header& header)
{
	const auto will_message_size = 0; // header.will ? size_of(header.will->second) : 0;
	const auto password_size     = 0; // header.password ? sizeof(*header.password) : 0;

	// check parameters
	if (!check_client_identifier(header.client_identifier)) {
		throw protocol_error{ "client identifier is invalid" };
	} else if (will_message_size > std::numeric_limits<std::uint16_t>::max()) {
		throw protocol_error{ "will message to large" };
	} else if (password_size > std::numeric_limits<std::uint16_t>::max()) {
		throw protocol_error{ "password to large" };
	}

	// fixed header & protocol name & level & connect flags
	const auto remaining_size = static_cast<variable_integer>(
	    12 + header.client_identifier.second /*+
	    (header.will ? header.will->first.second + 2 + will_message_size : 0) +
	    (header.username ? header.username->second : 0) + (header.password ? 2 + password_size : 0)*/);
	const auto protocol_level = byte{ 0x04 };
	const auto connect_flags  = static_cast<byte>(
        /*(header.username ? 0x80 : 0x00) | (header.password ? 0x40 : 0x00) | */ (header.will_retain << 5) |
        (static_cast<int>(header.qos) << 3) | /* (header.will ? 0x04 : 0x00) |*/ (header.clean_session << 1));

	write_elements(output, byte{ static_cast<int>(control_packet_type::connect) << 4 }, remaining_size,
	               std::uint16_t{ 4 }, byte{ 'M' }, byte{ 'Q' }, byte{ 'T' }, byte{ 'T' }, protocol_level,
	               connect_flags, header.keep_alive);

	// payload
	write(output, header.client_identifier);

	// 	if (header.will) {
	// 		write(output, header.will->first);
	// 		write_elements(output, static_cast<std::uint16_t>(will_message_size));

	// 		output << header.will->second.rdbuf();
	// 	}

	// 	if (header.username) {
	// 		write(output, *header.username);
	// 	}

	// 	if (header.password) {
	// 		write_elements(output, static_cast<std::uint16_t>(password_size));

	// 		output << header.password->rdbuf();
	// 	}
}

} // namespace detail
} // namespace mqtt

#endif
