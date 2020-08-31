#ifndef TERRAQTT_PROTOCOL_SUBSCRIPTION_HPP_
#define TERRAQTT_PROTOCOL_SUBSCRIPTION_HPP_

#include "general.hpp"
#include "reader.hpp"
#include "writer.hpp"

namespace terraqtt {

template<typename String>
struct subscribe_topic
{
	String filter;
	enum qos qos;
};

namespace protocol {

enum class suback_return_code
{
	success0 = 0x00,
	success1 = 0x01,
	success2 = 0x02,
	failure  = 0x80
};

/**
 * @tparam TopicContainer must hold the type subscribe_topic
 */
template<typename TopicContainer>
struct subscribe_header
{
	TopicContainer topics;
	std::uint16_t packet_identifier;
};

/**
 * @tparam ReturnCodeContainer must hold the type suback_return_code
 */
template<typename ReturnCodeContainer>
struct suback_header
{
	static_assert(std::is_same<typename ReturnCodeContainer::value_type, suback_return_code>::value,
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
 * @tparam TopicContainer must meet the requirements of *Container*
 */
template<typename Output, typename TopicContainer>
inline void write_packet(Output& output, std::error_code& ec, const subscribe_header<TopicContainer>& header)
{
	typename std::underlying_type<variable_integer>::type remaining = 2;

	for (const auto& i : header.topics) {
		if (!protected_add(remaining, 3u) || !protected_add(remaining, i.filter.size())) {
			ec = errc::payload_too_large;
			return;
		}
	}

	write_elements(output, ec,
	               static_cast<byte>(static_cast<int>(control_packet_type::subscribe) << 4 | 0x02),
	               static_cast<variable_integer>(remaining), header.packet_identifier);

	if (ec) {
		return;
	}

	for (const auto& i : header.topics) {
		if (write_blob<true>(output, ec, i.filter), ec) {
			return;
		}

		if (write_elements(output, ec, static_cast<byte>(i.qos)), ec) {
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
template<typename Output, typename ReturnCodeContainer>
inline void write_packet(Output& output, std::error_code& ec,
                         const suback_header<ReturnCodeContainer>& header)
{
	write_elements(output, ec, static_cast<byte>(static_cast<int>(control_packet_type::suback) << 4),
	               static_cast<variable_integer>(2 + header.return_codes.size()), header.packet_identifier);

	if (ec) {
		return;
	}

	for (auto i : header.return_codes) {
		if (write_elements(output, ec, static_cast<byte>(i)), ec) {
			return;
		}
	}
}

template<typename Input, typename ReturnCodeContainer>
inline bool read_packet(Input& input, std::error_code& ec, read_context& context,
                        suback_header<ReturnCodeContainer>& header)
{
	if (context.sequence == 0) {
		byte type;

		if (!read_element(input, ec, context, type) || ec) {
			return false;
		} else if (type != static_cast<byte>(static_cast<int>(control_packet_type::suback) << 4)) {
			ec = errc::bad_suback_flags;
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

	if ((context.sequence == 2 && !read_element(input, ec, context, header.packet_identifier)) || ec) {
		return false;
	}

	while (context.remaining_size) {
		byte rc;

		if (!read_element(input, ec, context, rc) || ec) {
			return false;
		}

		header.return_codes.push_back(static_cast<suback_return_code>(rc));
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
template<typename Output, typename TopicContainer>
inline void write_packet(Output& output, std::error_code& ec,
                         const unsubscribe_header<TopicContainer>& header)
{
	typename std::underlying_type<variable_integer>::type remaining = 2;

	for (const auto& i : header.topics) {
		if (!protected_add(remaining, 2u) || !protected_add(remaining, i.size())) {
			ec = errc::payload_too_large;
			return;
		}
	}

	write_elements(output, ec,
	               static_cast<byte>(static_cast<int>(control_packet_type::unsubscribe) << 4 | 0x02),
	               static_cast<variable_integer>(remaining), header.packet_identifier);

	if (!ec) {
		for (const auto& i : header.topics) {
			if (write_blob<true>(output, ec, i), ec) {
				return;
			}
		}
	}
}

template<typename Input>
inline bool read_packet(Input& input, std::error_code& ec, read_context& context, unsuback_header& header)
{
	if (context.sequence == 0) {
		byte type;

		if (!read_element(input, ec, context, type) || ec) {
			return false;
		} else if (type != static_cast<byte>(static_cast<int>(control_packet_type::unsuback) << 4)) {
			ec = errc::bad_unsuback_flags;
			return false;
		}
	}

	if (context.sequence == 1) {
		variable_integer remaining;

		if (!read_element(input, ec, context, remaining) || ec) {
			return false;
		} else if (remaining != static_cast<variable_integer>(2)) {
			ec = errc::bad_unsuback_payload;
			return false;
		}
	}

	return context.sequence == 2 && read_element(input, ec, context, header.packet_identifier);
}

} // namespace protocol
} // namespace terraqtt

#endif
