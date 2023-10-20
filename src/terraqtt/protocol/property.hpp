#pragma once

#include <cstdint>

namespace terraqtt::protocol {

enum class PropertyIdentifier : std::uint8_t {
	payload_format_indicator = 1,
	message_expiry_interval = 2,
	content_type = 3,

	response_topic = 8,
	correlation_data = 9,

	subscription_identifier = 11,

	session_expiry_interval = 17,
	assigned_client_identifier = 18,
	server_keep_alive = 19,

	authentication_method = 21,
	authentication_data = 22,
	request_problem_information = 23,
	will_delay_interval = 24,
	request_response_information = 25,
	response_information = 26,

	server_reference = 28,

	reason_string = 31,

	receive_maximum = 33,
	topic_alias_maximum = 34,
	topic_alias = 35,
	maximum_qos = 36,
	retain_available = 37,
	user_property = 38,
	maximum_packet_size = 39,
	wildcard_subscription_available = 40,
	subscription_identifier_available = 41,
	shared_subscription_available = 42,
};

} // namespace terraqtt::protocol
