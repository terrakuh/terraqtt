#ifndef TERRAQTT_PROTOCOL_PUBLISHING_HPP_
#define TERRAQTT_PROTOCOL_PUBLISHING_HPP_

#include "general.hpp"
#include "reader.hpp"
#include "writer.hpp"

namespace terraqtt {
namespace protocol {

template<typename String>
struct Publish_header {
	String topic;
	bool duplicate;
	bool retain;
	enum QoS qos;
	std::uint16_t packet_identifier;
};

struct Puback_header {
	std::uint16_t packet_identifier;
};

struct Pubrec_header {
	std::uint16_t packet_identifier;
};

struct pubrel_header {
	std::uint16_t packet_identifier;
};

struct Pubcomp_header {
	std::uint16_t packet_identifier;
};

template<typename Output, typename String, typename Payload>
inline void write_packet(Output& output, std::error_code& ec, const Publish_header<String>& header,
                         const Payload& payload)
{
	typename std::underlying_type<Variable_integer>::type remaining =
	  2 + (header.qos != QoS::at_most_once ? 2 : 0);

	if (!protected_add(remaining, header.topic.size()) || !protected_add(remaining, payload.size())) {
		ec = Error::payload_too_large;
		return;
	}

	write_elements(
	  output, ec,
	  static_cast<Byte>(static_cast<int>(Control_packet_type::publish) << 4 |
	                    (header.duplicate << 3 | static_cast<int>(header.qos) << 1 | header.retain)),
	  static_cast<Variable_integer>(remaining));

	if (ec || (write_blob<true>(output, ec, header.topic), ec)) {
		return;
	}

	if (header.qos != QoS::at_most_once) {
		if (write_elements(output, ec, header.packet_identifier), ec) {
			return;
		}
	}

	write_blob<false>(output, ec, payload);
}

template<typename Input, typename String>
inline bool read_packet(Input& input, std::error_code& ec, Read_context& context,
                        Publish_header<String>& header, Variable_integer_type& payload_size)
{
	if (context.sequence == 0) {
		Byte type;
		if (!read_element(input, ec, context, type) || ec) {
			return false;
		}
		header.duplicate = type >> 3 & 1;
		header.qos       = static_cast<QoS>(type >> 1 & 0x03);
		header.retain    = type & 1;
		if (header.qos != QoS::at_most_once && header.qos != QoS::at_least_once &&
		    header.qos != QoS::exactly_once) {
			ec = Error::bad_qos;
			return false;
		}
	}

	if (context.sequence == 1) {
		Variable_integer remaining;
		if (!read_element(input, ec, context, remaining) || ec) {
			return false;
		}
		context.remaining_size = static_cast<Variable_integer_type>(remaining);
	}

	if ((context.sequence == 2 && !read_blob(input, ec, context, header.topic)) || ec) {
		return false;
	}

	if (header.qos != QoS::at_most_once) {
		if (!read_element(input, ec, context, header.packet_identifier) || ec) {
			return false;
		}
	}

	payload_size = context.remaining_size;
	return true;
}

template<typename Output>
inline void write_packet(Output& output, std::error_code& ec, const Puback_header& header)
{
	write_elements(output, ec, static_cast<Byte>(static_cast<int>(Control_packet_type::puback) << 4),
	               static_cast<Variable_integer>(2), header.packet_identifier);
}

template<typename Input>
inline bool read_packet(Input& input, std::error_code& ec, Read_context& context, Puback_header& header)
{
	if (context.sequence == 0) {
		Byte type;
		if (!read_element(input, ec, context, type) || ec) {
			return false;
		} else if (type != static_cast<Byte>(static_cast<int>(Control_packet_type::puback) << 4)) {
			ec = Error::bad_puback_flags;
			return false;
		}
	}

	if (context.sequence == 1) {
		Variable_integer remaining;
		if (!read_element(input, ec, context, remaining) || ec) {
			return false;
		} else if (remaining != static_cast<Variable_integer>(2)) {
			ec = Error::bad_puback_payload;
			return false;
		}
		context.remaining_size = 2;
	}

	return read_element(input, ec, context, header.packet_identifier);
}

template<typename Output>
inline void write_packet(Output& output, std::error_code& ec, const Pubrec_header& header)
{
	write_elements(output, ec, static_cast<Byte>(static_cast<int>(Control_packet_type::pubrec) << 4),
	               static_cast<Variable_integer>(2), header.packet_identifier);
}

template<typename Input>
inline bool read_packet(Input& input, std::error_code& ec, Read_context& context, Pubrec_header& header)
{
	if (context.sequence == 0) {
		Byte type;
		if (!read_element(input, ec, context, type) || ec) {
			return false;
		} else if (type != static_cast<Byte>(static_cast<int>(Control_packet_type::pubrec) << 4)) {
			ec = Error::bad_pubrec_flags;
			return false;
		}
	}

	if (context.sequence == 1) {
		Variable_integer remaining;
		if (!read_element(input, ec, context, remaining) || ec) {
			return false;
		} else if (remaining != static_cast<Variable_integer>(2)) {
			ec = Error::bad_pubrec_payload;
			return false;
		}
		context.remaining_size = 2;
	}

	return read_element(input, ec, context, header.packet_identifier);
}

template<typename Output>
inline void write_packet(Output& output, std::error_code& ec, const pubrel_header& header)
{
	write_elements(output, ec, static_cast<Byte>(static_cast<int>(Control_packet_type::pubrel) << 4),
	               static_cast<Variable_integer>(2), header.packet_identifier);
}

template<typename Input>
inline bool read_packet(Input& input, std::error_code& ec, Read_context& context, pubrel_header& header)
{
	if (context.sequence == 0) {
		Byte type;
		if (!read_element(input, ec, context, type) || ec) {
			return false;
		} else if (type != static_cast<Byte>(static_cast<int>(Control_packet_type::pubrel) << 4)) {
			ec = Error::bad_pubrel_flags;
			return false;
		}
	}

	if (context.sequence == 1) {
		Variable_integer remaining;
		if (!read_element(input, ec, context, remaining) || ec) {
			return false;
		} else if (remaining != static_cast<Variable_integer>(2)) {
			ec = Error::bad_pubrel_payload;
			return false;
		}
		context.remaining_size = 2;
	}

	return read_element(input, ec, context, header.packet_identifier);
}

template<typename Output>
inline void write_packet(Output& output, std::error_code& ec, const Pubcomp_header& header)
{
	write_elements(output, ec, static_cast<Byte>(static_cast<int>(Control_packet_type::pubcomp) << 4),
	               static_cast<Variable_integer>(2), header.packet_identifier);
}

template<typename Input>
inline bool read_packet(Input& input, std::error_code& ec, Read_context& context, Pubcomp_header& header)
{
	if (context.sequence == 0) {
		Byte type;
		if (!read_element(input, ec, context, type) || ec) {
			return false;
		} else if (type != static_cast<Byte>(static_cast<int>(Control_packet_type::pubcomp) << 4)) {
			ec = Error::bad_pubcomp_flags;
			return false;
		}
	}

	if (context.sequence == 1) {
		Variable_integer remaining;
		if (!read_element(input, ec, context, remaining) || ec) {
			return false;
		} else if (remaining != static_cast<Variable_integer>(2)) {
			ec = Error::bad_pubcomp_payload;
			return false;
		}
		context.remaining_size = 2;
	}

	return read_element(input, ec, context, header.packet_identifier);
}

} // namespace protocol
} // namespace terraqtt

#endif
