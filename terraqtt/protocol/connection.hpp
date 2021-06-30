#ifndef TERRAQTT_PROTOCOL_CONNECTION_HPP_
#define TERRAQTT_PROTOCOL_CONNECTION_HPP_

#include "general.hpp"
#include "reader.hpp"
#include "writer.hpp"

#include <limits>
#include <type_traits>
#include <utility>

namespace terraqtt {
namespace protocol {

enum class Connack_return_code
{
	accepted,
	unacceptable_version,
	identifier_rejected,
	server_unavailable,
	bad_user_password,
	not_authorized
};

template<typename String, typename Will_message, typename Password>
struct Connect_header
{
	String client_identifier;
	std::pair<String, Will_message>* will;
	typename std::remove_reference<String>::type* username;
	typename std::remove_reference<Password>::type* password;
	QoS will_qos;
	bool will_retain;
	bool clean_session;
	/// the keep alive timeout in seconds; 0 means no timeout
	std::uint16_t keep_alive;
};

struct Connack_header
{
	bool session_present;
	Connack_return_code return_code;
};

struct Disconnect_header
{};

template<typename Output, typename String, typename Will_message, typename Password>
inline void write_packet(Output& output, std::error_code& ec,
                         const Connect_header<String, Will_message, Password>& header)
{
	if (!header.will && (header.will_qos != QoS::at_most_once || header.will_retain)) {
		ec = Error::bad_will;
		return;
	} else if (!header.username && header.password) {
		ec = Error::bad_username_password;
		return;
	} else if (!header.clean_session && !header.client_identifier.size()) {
		ec = Error::empty_client_identifier;
		return;
	}

	// fixed header & protocol name & level & connect flags
	typename std::underlying_type<Variable_integer>::type remaining =
	    12 + (header.will ? 4 : 0) + (header.username ? 2 : 0) + (header.password ? 2 : 0);
	const auto protocol_level = Byte{ 0x04 };
	const auto connect_flags  = static_cast<Byte>(
        (header.username ? 0x80 : 0x00) | (header.password ? 0x40 : 0x00) | (header.will_retain << 5) |
        (static_cast<int>(header.will_qos) << 3) | (header.will ? 0x04 : 0x00) | (header.clean_session << 1));

	if (!protected_add(remaining, header.client_identifier.size()) ||
	    (header.will && !protected_add(remaining, header.will->first.size())) ||
	    (header.will && !protected_add(remaining, header.will->second.size())) ||
	    (header.username && !protected_add(remaining, header.username->size())) ||
	    (header.password && !protected_add(remaining, header.password->size()))) {
		ec = Error::payload_too_large;
		return;
	}

	write_elements(output, ec, Byte{ static_cast<int>(Control_packet_type::connect) << 4 },
	               static_cast<Variable_integer>(remaining), std::uint16_t{ 4 }, Byte{ 'M' }, Byte{ 'Q' },
	               Byte{ 'T' }, Byte{ 'T' }, protocol_level, connect_flags, header.keep_alive);

	if (ec) {
		return;
	}

	// payload
	if (write_blob<true>(output, ec, header.client_identifier), ec) {
		return;
	}

	if (header.will) {
		if (write_blob<true>(output, ec, header.will->first), ec) {
			return;
		}

		if (write_blob<true>(output, ec, header.will->second), ec) {
			return;
		}
	}

	if (header.username) {
		if (write_blob<true>(output, ec, *header.username), ec) {
			return;
		}
	}

	if (header.password) {
		if (write_blob<true>(output, ec, *header.password), ec) {
			return;
		}
	}
}

template<typename Input>
inline bool read_packet(Input& input, std::error_code& ec, Read_context& context, Connack_header& header)
{
	if (context.sequence == 0) {
		Byte type;
		if (!read_element(input, ec, context, type) || ec) {
			return false;
		} else if (type != static_cast<Byte>(static_cast<int>(Control_packet_type::connack) << 4)) {
			ec = Error::bad_connack_flags;
			return false;
		}
	}

	if (context.sequence == 1) {
		Variable_integer remaining;
		if (!read_element(input, ec, context, remaining) || ec) {
			return false;
		} else if (remaining != static_cast<Variable_integer>(2)) {
			ec = Error::bad_connack_payload;
			return false;
		}
	}

	if (context.sequence == 2) {
		Byte flags;
		if (!read_element(input, ec, context, flags) || ec) {
			return false;
		} else if (flags & 0xfe) {
			ec = Error::bad_connack_flags;
			return false;
		}
		header.session_present = flags & 0x01;
	}

	if (context.sequence == 3) {
		Byte rc;
		if (!read_element(input, ec, context, rc) || ec) {
			return false;
		} else if (rc > static_cast<Byte>(Connack_return_code::not_authorized)) {
			ec = Error::bad_connack_return_code;
			return false;
		}
		header.return_code = static_cast<Connack_return_code>(rc);
	}

	return true;
}

/**
 * Writes a disconnect packet to the output stream.
 *
 * @exception see write_elements()
 * @param output[in] the output stream
 * @param header the disconnect header
 */
template<typename Output>
inline void write_packet(Output& output, std::error_code& ec, const Disconnect_header& header)
{
	write_elements(output, ec, static_cast<Byte>(static_cast<int>(Control_packet_type::disconnect) << 4),
	               static_cast<Variable_integer>(0));
}

} // namespace protocol
} // namespace terraqtt

#endif
