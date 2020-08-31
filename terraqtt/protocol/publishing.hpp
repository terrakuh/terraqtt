#ifndef TERRAQTT_PROTOCOL_PUBLISHING_HPP_
#define TERRAQTT_PROTOCOL_PUBLISHING_HPP_

#include "general.hpp"
#include "reader.hpp"
#include "writer.hpp"

namespace terraqtt {
namespace protocol {

template<typename String>
struct publish_header
{
	String topic;
	bool duplicate;
	bool retain;
	enum qos qos;
	std::uint16_t packet_identifier;
};

struct puback_header
{
	std::uint16_t packet_identifier;
};

struct pubrec_header
{
	std::uint16_t packet_identifier;
};

struct pubrel_header
{
	std::uint16_t packet_identifier;
};

struct pubcomp_header
{
	std::uint16_t packet_identifier;
};

template<typename Output, typename String, typename Payload>
inline void write_packet(Output& output, std::error_code& ec, const publish_header<String>& header,
                         const Payload& payload)
{
	typename std::underlying_type<variable_integer>::type remaining =
	    2 + (header.qos != qos::at_most_once ? 2 : 0);

	if (!protected_add(remaining, header.topic.size()) || !protected_add(remaining, payload.size())) {
		ec = errc::payload_too_large;
		return;
	}

	write_elements(
	    output, ec,
	    static_cast<byte>(static_cast<int>(control_packet_type::publish) << 4 |
	                      (header.duplicate << 3 | static_cast<int>(header.qos) << 1 | header.retain)),
	    static_cast<variable_integer>(remaining));

	if (ec || (write_blob<true>(output, ec, header.topic), ec)) {
		return;
	}

	if (header.qos != qos::at_most_once) {
		if (write_elements(output, ec, header.packet_identifier), ec) {
			return;
		}
	}

	write_blob<false>(output, ec, payload);
}

template<typename Input, typename String>
inline bool read_packet(Input& input, std::error_code& ec, read_context& context,
                        publish_header<String>& header, variable_integer_type& payload_size)
{
	if (context.sequence == 0) {
		byte type;

		if (!read_element(input, ec, context, type) || ec) {
			return false;
		}

		header.duplicate = type >> 3 & 1;
		header.qos       = static_cast<qos>(type >> 1 & 0x03);
		header.retain    = type & 1;

		if (header.qos != qos::at_most_once && header.qos != qos::at_least_once &&
		    header.qos != qos::exactly_once) {
			ec = errc::bad_qos;
			return false;
		}
	}

	if (context.sequence == 1) {
		variable_integer remaining;

		if (!read_element(input, ec, context, remaining) || ec) {
			return false;
		}

		context.remaining_size = static_cast<variable_integer_type>(remaining);
	}

	if ((context.sequence == 2 && !read_blob(input, ec, context, header.topic)) || ec) {
		return false;
	}

	if (header.qos != qos::at_most_once) {
		if (!read_element(input, ec, context, header.packet_identifier) || ec) {
			return false;
		}
	}

	payload_size = context.remaining_size;

	return true;
}

template<typename Output>
inline void write_packet(Output& output, std::error_code& ec, const puback_header& header)
{
	write_elements(output, ec, static_cast<byte>(static_cast<int>(control_packet_type::puback) << 4),
	               static_cast<variable_integer>(2), header.packet_identifier);
}

template<typename Input>
inline bool read_packet(Input& input, std::error_code& ec, read_context& context, puback_header& header)
{
	if (context.sequence == 0) {
		byte type;

		if (!read_element(input, ec, context, type) || ec) {
			return false;
		} else if (type != static_cast<byte>(static_cast<int>(control_packet_type::puback) << 4)) {
			ec = errc::bad_puback_flags;
			return false;
		}
	}

	if (context.sequence == 1) {
		variable_integer remaining;

		if (!read_element(input, ec, context, remaining) || ec) {
			return false;
		} else if (remaining != static_cast<variable_integer>(2)) {
			ec = errc::bad_puback_payload;
			return false;
		}

		context.remaining_size = 2;
	}

	return read_element(input, ec, context, header.packet_identifier);
}

template<typename Output>
inline void write_packet(Output& output, std::error_code& ec, const pubrec_header& header)
{
	write_elements(output, ec, static_cast<byte>(static_cast<int>(control_packet_type::pubrec) << 4),
	               static_cast<variable_integer>(2), header.packet_identifier);
}

template<typename Input>
inline bool read_packet(Input& input, std::error_code& ec, read_context& context, pubrec_header& header)
{
	if (context.sequence == 0) {
		byte type;

		if (!read_element(input, ec, context, type) || ec) {
			return false;
		} else if (type != static_cast<byte>(static_cast<int>(control_packet_type::pubrec) << 4)) {
			ec = errc::bad_pubrec_flags;
			return false;
		}
	}

	if (context.sequence == 1) {
		variable_integer remaining;

		if (!read_element(input, ec, context, remaining) || ec) {
			return false;
		} else if (remaining != static_cast<variable_integer>(2)) {
			ec = errc::bad_pubrec_payload;
			return false;
		}

		context.remaining_size = 2;
	}

	return read_element(input, ec, context, header.packet_identifier);
}

template<typename Output>
inline void write_packet(Output& output, std::error_code& ec, const pubrel_header& header)
{
	write_elements(output, ec, static_cast<byte>(static_cast<int>(control_packet_type::pubrel) << 4),
	               static_cast<variable_integer>(2), header.packet_identifier);
}

template<typename Input>
inline bool read_packet(Input& input, std::error_code& ec, read_context& context, pubrel_header& header)
{
	if (context.sequence == 0) {
		byte type;

		if (!read_element(input, ec, context, type) || ec) {
			return false;
		} else if (type != static_cast<byte>(static_cast<int>(control_packet_type::pubrel) << 4)) {
			ec = errc::bad_pubrel_flags;
			return false;
		}
	}

	if (context.sequence == 1) {
		variable_integer remaining;

		if (!read_element(input, ec, context, remaining) || ec) {
			return false;
		} else if (remaining != static_cast<variable_integer>(2)) {
			ec = errc::bad_pubrel_payload;
			return false;
		}

		context.remaining_size = 2;
	}

	return read_element(input, ec, context, header.packet_identifier);
}

template<typename Output>
inline void write_packet(Output& output, std::error_code& ec, const pubcomp_header& header)
{
	write_elements(output, ec, static_cast<byte>(static_cast<int>(control_packet_type::pubcomp) << 4),
	               static_cast<variable_integer>(2), header.packet_identifier);
}

template<typename Input>
inline bool read_packet(Input& input, std::error_code& ec, read_context& context, pubcomp_header& header)
{
	if (context.sequence == 0) {
		byte type;

		if (!read_element(input, ec, context, type) || ec) {
			return false;
		} else if (type != static_cast<byte>(static_cast<int>(control_packet_type::pubcomp) << 4)) {
			ec = errc::bad_pubcomp_flags;
			return false;
		}
	}

	if (context.sequence == 1) {
		variable_integer remaining;

		if (!read_element(input, ec, context, remaining) || ec) {
			return false;
		} else if (remaining != static_cast<variable_integer>(2)) {
			ec = errc::bad_pubcomp_payload;
			return false;
		}

		context.remaining_size = 2;
	}

	return read_element(input, ec, context, header.packet_identifier);
}

} // namespace protocol
} // namespace terraqtt

#endif
