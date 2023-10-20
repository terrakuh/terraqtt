#pragma once

#include "../io/reader.hpp"

#include <bitset>
#include <concepts>
#include <cstdint>


namespace terraqtt::protocol::v5 {

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

template<typename Type>
struct Property {
	bool include;
	PropertyIdentifier identifier;
	Type value;
};

enum class ReasonCode : std::uint8_t {
	success = 0,
	normal_disconnection = 0,
	granted_qos_0 = 0,
	granted_qos_1 = 1,
	granted_qos_2 = 2,
	disconnect_with_will_message = 4,
	no_matching_subscribers = 16,
	no_subscription_existed = 17,
	continue_authentication = 24,
	reauthenticate = 25,

	unspecified_error = 128,
	malformed_packet = 129,
	protocol_error = 130,
	implementation_specific_error = 131,
	unsupported_protocol_version = 132,
	invalid_client_identifier = 133,
	bad_username_password = 134,
	not_authorized = 135,
	server_unavailable = 136,
	server_busy = 137,
	banned = 138,
	server_shutting_down = 139,
	bad_authentication_method = 140,
	keep_alive_timeout = 141,
	session_taken_over = 142,
	invalid_topic_filter = 143,
	invalid_topic_name = 144,
	packet_id_in_use = 145,
	packet_id_not_found = 146,
	receive_max_exceeded = 147,
	invalid_topic_alias = 148,
	packet_too_large = 149,
	message_rate_too_high = 150,
	quota_exceeded = 151,
	administrative_action = 152,
	invalid_payload_format = 153,
	retain_not_supported = 154,
	qos_not_supported = 155,
	use_another_server = 156,
	server_moved = 157,
	unsupported_shared_subscriptions = 158,
	connection_rate_exceeded = 159,
	max_connect_time = 160,
	unsupported_subscription_id = 161,
	unsupported_wildcard_subscription = 162,
};

template<bool Conservative, io::AsyncContext Context, std::invocable<PropertyIdentifier> Callback>
inline typename Context::template return_type<std::size_t>
  read_properties(io::Reader<5, Conservative, Context>& reader,
                  std::underlying_type_t<VariableInteger> property_length, Callback&& callback)
{
	std::error_code ec{};
	std::size_t total_read = 0;
	const auto old_remaining_length = reader.remaining_length();

	if (old_remaining_length < property_length) {
		// TODO problem
	}

	while (property_length > old_remaining_length - reader.remaining_length()) {
		PropertyIdentifier identifier{};
		std::size_t read = 0;
		std::tie(ec, read) = co_await reader.read_fields(identifier);
		total_read += read;

		std::tie(ec, read) = co_await callback(identifier);
		total_read += read;
	}

	if (property_length < old_remaining_length - reader.remaining_length()) {
		// TODO problem
	}

	co_return reader.context().return_value(ec, total_read);
}

} // namespace terraqtt::protocol::v5
