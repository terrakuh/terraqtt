#ifndef MQTT_PROTOCOL_PUBLISHING_HPP_
#define MQTT_PROTOCOL_PUBLISHING_HPP_

#include "general.hpp"
#include "reader.hpp"
#include "writer.hpp"

namespace mqtt {
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

template<typename String, typename Payload>
inline void write_packet(byte_ostream& output, const publish_header<String>& header, const Payload& payload)
{
	typename std::underlying_type<variable_integer>::type remaining =
	    2 + (header.qos != qos::at_most_once ? 2 : 0);

	if (!protected_add(remaining, header.topic.size()) || !protected_add(remaining, payload.size())) {
		throw protocol_error{ "remaining size cannot be represented" };
	}

	write_elements(
	    output,
	    static_cast<byte>(static_cast<int>(control_packet_type::publish) << 4 |
	                      (header.duplicate << 3 | static_cast<int>(header.qos) << 1 | header.retain)),
	    static_cast<variable_integer>(remaining));
	write_blob<true>(output, header.topic);

	if (header.qos != qos::at_most_once) {
		write_elements(output, header.packet_identifier);
	}

	write_blob<false>(output, payload);
}

template<typename String>
inline bool read_packet(byte_istream& input, read_context& context, publish_header<String>& header,
                        variable_integer_type& payload_size)
{
	if (context.sequence == 0) {
		byte type;

		if (!read_element(input, context, type)) {
			return false;
		}

		header.duplicate = type >> 3 & 1;
		header.qos       = static_cast<qos>(type >> 1 & 0x03);
		header.retain    = type & 1;

		if (header.qos != qos::at_most_once && header.qos != qos::at_least_once &&
		    header.qos != qos::exactly_once) {
			throw protocol_error{ "invalid QoS" };
		}
	}

	if (context.sequence == 1) {
		variable_integer remaining;

		if (!read_element(input, context, remaining)) {
			return false;
		}

		context.remaining_size = static_cast<variable_integer_type>(remaining);
	}

	if (context.sequence == 2 && !read_blob(input, context, header.topic)) {
		return false;
	}

	if (header.qos != qos::at_most_once) {
		if (!read_element(input, context, header.packet_identifier)) {
			return false;
		}
	}

	payload_size = context.remaining_size;

	return true;
}

inline void write_packet(byte_ostream& output, const puback_header& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::puback) << 4),
	               static_cast<variable_integer>(2), header.packet_identifier);
}

inline bool read_packet(byte_istream& input, read_context& context, puback_header& header)
{
	if (context.sequence == 0) {
		byte type;

		if (!read_element(input, context, type)) {
			return false;
		} else if (type != static_cast<byte>(static_cast<int>(control_packet_type::puback) << 4)) {
			throw protocol_error{ "invalid puback flags" };
		}
	}

	if (context.sequence == 1) {
		variable_integer remaining;

		if (!read_element(input, context, remaining)) {
			return false;
		} else if (remaining != static_cast<variable_integer>(2)) {
			throw protocol_error{ "invalid puback payload" };
		}

		context.remaining_size = 2;
	}

	return read_element(input, context, header.packet_identifier);
}

inline void write_packet(byte_ostream& output, const pubrec_header& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::pubrec) << 4),
	               static_cast<variable_integer>(2), header.packet_identifier);
}

inline bool read_packet(byte_istream& input, read_context& context, pubrec_header& header)
{
	if (context.sequence == 0) {
		byte type;

		if (!read_element(input, context, type)) {
			return false;
		} else if (type != static_cast<byte>(static_cast<int>(control_packet_type::pubrec) << 4)) {
			throw protocol_error{ "invalid pubrec flags" };
		}
	}

	if (context.sequence == 1) {
		variable_integer remaining;

		if (!read_element(input, context, remaining)) {
			return false;
		} else if (remaining != static_cast<variable_integer>(2)) {
			throw protocol_error{ "invalid pubrec payload" };
		}

		context.remaining_size = 2;
	}

	return read_element(input, context, header.packet_identifier);
}

inline void write_packet(byte_ostream& output, const pubrel_header& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::pubrel) << 4),
	               static_cast<variable_integer>(2), header.packet_identifier);
}

inline bool read_packet(byte_istream& input, read_context& context, pubrel_header& header)
{
	if (context.sequence == 0) {
		byte type;

		if (!read_element(input, context, type)) {
			return false;
		} else if (type != static_cast<byte>(static_cast<int>(control_packet_type::pubrel) << 4)) {
			throw protocol_error{ "invalid pubrel flags" };
		}
	}

	if (context.sequence == 1) {
		variable_integer remaining;

		if (!read_element(input, context, remaining)) {
			return false;
		} else if (remaining != static_cast<variable_integer>(2)) {
			throw protocol_error{ "invalid pubrel payload" };
		}

		context.remaining_size = 2;
	}

	return read_element(input, context, header.packet_identifier);
}

inline void write_packet(byte_ostream& output, const pubcomp_header& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::pubcomp) << 4),
	               static_cast<variable_integer>(2), header.packet_identifier);
}

inline bool read_packet(byte_istream& input, read_context& context, pubcomp_header& header)
{
	if (context.sequence == 0) {
		byte type;

		if (!read_element(input, context, type)) {
			return false;
		} else if (type != static_cast<byte>(static_cast<int>(control_packet_type::pubcomp) << 4)) {
			throw protocol_error{ "invalid pubcomp flags" };
		}
	}

	if (context.sequence == 1) {
		variable_integer remaining;

		if (!read_element(input, context, remaining)) {
			return false;
		} else if (remaining != static_cast<variable_integer>(2)) {
			throw protocol_error{ "invalid pubcomp payload" };
		}

		context.remaining_size = 2;
	}

	return read_element(input, context, header.packet_identifier);
}

} // namespace protocol
} // namespace mqtt

#endif
