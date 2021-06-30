#ifndef TERRAQTT_PROTOCOL_SUBSCRIPTION_HPP_
#define TERRAQTT_PROTOCOL_SUBSCRIPTION_HPP_

#include "general.hpp"
#include "reader.hpp"
#include "writer.hpp"

namespace terraqtt {

template<typename String>
struct Subscribe_topic
{
	String filter;
	QoS qos;
};

namespace protocol {

enum class Suback_return_code
{
	success0 = 0x00,
	success1 = 0x01,
	success2 = 0x02,
	failure  = 0x80
};

/**
 * @tparam TopicContainer must hold the type subscribe_topic
 */
template<typename Topic_container>
struct Subscribe_header
{
	Topic_container topics;
	std::uint16_t packet_identifier;
};

/**
 * @tparam ReturnCodeContainer must hold the type suback_return_code
 */
template<typename Return_code_container>
struct Suback_header
{
	static_assert(std::is_same<typename Return_code_container::value_type, Suback_return_code>::value,
	              "Return_code_container must return return_code");

	Return_code_container return_codes;
	std::uint16_t packet_identifier;
};

/**
 * @tparam TopicContainer must hold Strings
 */
template<typename Topic_container>
struct Unsubscribe_header
{
	Topic_container topics;
	std::uint16_t packet_identifier;
};

struct Unsuback_header
{
	std::uint16_t packet_identifier;
};

/**
 * Writes a subscribe packet to the output stream.
 *
 * @exception see write_blob(), write_elements()
 * @param output[in] the output stream
 * @param header the subscribe header
 * @tparam TopicContainer must meet the requirements of *Container*
 */
template<typename Output, typename Topic_container>
inline void write_packet(Output& output, std::error_code& ec, const Subscribe_header<Topic_container>& header)
{
	typename std::underlying_type<Variable_integer>::type remaining = 2;
	for (const auto& i : header.topics) {
		if (!protected_add(remaining, 3u) || !protected_add(remaining, i.filter.size())) {
			ec = Error::payload_too_large;
			return;
		}
	}

	write_elements(output, ec,
	               static_cast<Byte>(static_cast<int>(Control_packet_type::subscribe) << 4 | 0x02),
	               static_cast<Variable_integer>(remaining), header.packet_identifier);
	if (ec) {
		return;
	}

	for (const auto& i : header.topics) {
		if (write_blob<true>(output, ec, i.filter), ec) {
			return;
		}

		if (write_elements(output, ec, static_cast<Byte>(i.qos)), ec) {
			return;
		}
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
template<typename Output, typename Return_code_container>
inline void write_packet(Output& output, std::error_code& ec,
                         const Suback_header<Return_code_container>& header)
{
	write_elements(output, ec, static_cast<Byte>(static_cast<int>(Control_packet_type::suback) << 4),
	               static_cast<Variable_integer>(2 + header.return_codes.size()), header.packet_identifier);
	if (ec) {
		return;
	}

	for (auto i : header.return_codes) {
		if (write_elements(output, ec, static_cast<Byte>(i)), ec) {
			return;
		}
	}
}

template<typename Input, typename Return_code_container>
inline bool read_packet(Input& input, std::error_code& ec, Read_context& context,
                        Suback_header<Return_code_container>& header)
{
	if (context.sequence == 0) {
		Byte type;
		if (!read_element(input, ec, context, type) || ec) {
			return false;
		} else if (type != static_cast<Byte>(static_cast<int>(Control_packet_type::suback) << 4)) {
			ec = Error::bad_suback_flags;
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

	if ((context.sequence == 2 && !read_element(input, ec, context, header.packet_identifier)) || ec) {
		return false;
	}

	while (context.remaining_size) {
		Byte rc;
		if (!read_element(input, ec, context, rc) || ec) {
			return false;
		}
		header.return_codes.push_back(static_cast<Suback_return_code>(rc));
	}
	return true;
}

/**
 * Writes a unsubscribe packet to the output stream.
 *
 * @exception see write_blob(), write_elements()
 * @param output[in] the output stream
 * @param header the unsubscribe header
 * @tparam TopicContainer must meet the requirements of *Container*
 */
template<typename Output, typename Topic_container>
inline void write_packet(Output& output, std::error_code& ec,
                         const Unsubscribe_header<Topic_container>& header)
{
	typename std::underlying_type<Variable_integer>::type remaining = 2;
	for (const auto& i : header.topics) {
		if (!protected_add(remaining, 2u) || !protected_add(remaining, i.size())) {
			ec = Error::payload_too_large;
			return;
		}
	}

	write_elements(output, ec,
	               static_cast<Byte>(static_cast<int>(Control_packet_type::unsubscribe) << 4 | 0x02),
	               static_cast<Variable_integer>(remaining), header.packet_identifier);
	if (!ec) {
		for (const auto& i : header.topics) {
			if (write_blob<true>(output, ec, i), ec) {
				return;
			}
		}
	}
}

template<typename Input>
inline bool read_packet(Input& input, std::error_code& ec, Read_context& context, Unsuback_header& header)
{
	if (context.sequence == 0) {
		Byte type;
		if (!read_element(input, ec, context, type) || ec) {
			return false;
		} else if (type != static_cast<Byte>(static_cast<int>(Control_packet_type::unsuback) << 4)) {
			ec = Error::bad_unsuback_flags;
			return false;
		}
	}

	if (context.sequence == 1) {
		Variable_integer remaining;
		if (!read_element(input, ec, context, remaining) || ec) {
			return false;
		} else if (remaining != static_cast<Variable_integer>(2)) {
			ec = Error::bad_unsuback_payload;
			return false;
		}
	}

	return context.sequence == 2 && read_element(input, ec, context, header.packet_identifier);
}

} // namespace protocol
} // namespace terraqtt

#endif
