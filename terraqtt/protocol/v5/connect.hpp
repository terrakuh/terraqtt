#pragma once

#include "fixed_header.hpp"

#include <chrono>

namespace terraqtt::protocol::v5 {

using KeepAliveDuration     = std::chrono::duration<std::uint16_t>;
using SessionExpiryInterval = std::chrono::duration<std::uint32_t>;

struct ConnectHeader {
	bool clean_start;
	// TODO Add username/password.
	// TODO Add will.
	KeepAliveDuration keep_alive;
	SessionExpiryInterval session_expiry;
	std::uint16_t receive_maximum;
	std::uint32_t max_packet_size;
	std::uint16_t topic_alias_maximum;
	bool request_response_information;
	bool request_problem_information;
	// TODO User property.
	// TODO Authentication method and data.
};

template<AsyncContext Context>
inline typename Context::template return_type<std::size_t> write(Context& context,
                                                                 const ConnectHeader& header)
{
	co_return co_await write_whole_packet(
	  context, ControlPacketType::connect, 0, 0, std::string_view{ "MQTT" }, std::uint8_t{ 5 },
	  static_cast<std::uint8_t>(header.clean_start << 1), header.keep_alive.count(),
	  PropertyIdentifier::session_expiry_interval, header.session_expiry.count(),
	  PropertyIdentifier::receive_maximum, header.receive_maximum, PropertyIdentifier::maximum_packet_size,
	  header.max_packet_size, PropertyIdentifier::topic_alias_maximum, header.topic_alias_maximum,
	  PropertyIdentifier::request_response_information, header.request_response_information,
	  PropertyIdentifier::request_problem_information, header.request_problem_information);
}

} // namespace terraqtt::protocol::v5
