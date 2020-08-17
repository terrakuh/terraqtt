#ifndef MQTT_DETAIL_CONNECT_HPP_
#define MQTT_DETAIL_CONNECT_HPP_

#include "../optional.hpp"
#include "general.hpp"

#include <utility>
#include <limits>

namespace mqtt {
namespace detail {

struct connect_header
{
	typedef std::pair<std::string, byte_istream&> will_type;

	enum qos qos;
	bool will_retain;
	bool clean_session;
	/** the keep alive timeout in seconds; 0 means no timeout */
	std::uint16_t keep_alive;
	/** must be between 1-23 and may only contain
	 * "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" */
	std::string client_identifier;
	optional<will_type> will;
	optional<std::string> username;
	optional<byte_istream&> password;
};

inline bool check_client_identifier(const std::string& identifier)
{
	for (auto i : identifier) {
		if (!std::isalnum(i)) {
			return false;
		}
	}

	return !identifier.empty();
}

template<>
inline void write<control_packet_type::connect, const connect_header&>(byte_ostream& output,
                                                                       const connect_header& header)
{
	const auto will_message_size =
	    header.will.map([](const connect_header::will_type& will) { return size_of(will.second); })
	        .or_else(0);
	const auto password_size = header.password.map(&size_of).or_else(0);

	// check parameters
	if (check_client_identifier(header.client_identifier)) {
		throw protocol_error{ "client identifier is invalid" };
	} else if (will_message_size > std::numeric_limits<std::uint16_t>::max()) {
		throw protocol_error{ "will message to large" };
	} else if (password_size > std::numeric_limits<std::uint16_t>::max()) {
		throw protocol_error{ "password to large" };
	}

	write_fixed_header<control_packet_type::connect>(
	    output, 10 + header.client_identifier.length() +
	                (header.will ? header.will->first.length() + 2 + will_message_size : 0) +
	                (header.username ? header.username->length() : 0) +
	                (header.password ? 2 + password_size : 0));

	// protocol name & level & connect flags
	const auto protocol_level = 0x04;
	const auto connect_flags  = (header.username ? 0x80 : 0x00) | (header.password ? 0x40 : 0x00) |
	                           (header.will_retain << 5) | (static_cast<int>(header.qos) << 3) |
	                           (header.will ? 0x04 : 0x00) | (header.clean_session << 1);

	write_bytes(output, 0x00, 0x04, 'M', 'Q', 'T', 'T', protocol_level, connect_flags);

	// keep alive
	write_byte_sequence(output, to_byte_sequence(header.keep_alive));

	// payload
	write_utf8(output, header.client_identifier);
	header.will.if_present([&](const connect_header::will_type& will) {
		write_utf8(output, will.first);
		write_byte_sequence(output, to_byte_sequence(static_cast<std::uint16_t>(will_message_size)));

		output << will.second.rdbuf();
	});
	header.username.if_present([&](const std::string& username) { write_utf8(output, username); });
	header.password.if_present([&](byte_istream& password) {
		write_byte_sequence(output, to_byte_sequence(static_cast<std::uint16_t>(password_size)));

		output << password.rdbuf();
	});
}

} // namespace detail
} // namespace mqtt

#endif
