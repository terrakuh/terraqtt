#pragma once

#include <system_error>
#include <type_traits>

namespace terraqtt {

enum class Error {
	success,

	// variable_integer_too_large = 1,
	payload_too_large,
	string_too_long,
	bad_remaining_size,

	// Protocol errors from 100 - 5000.
	malformed_variable_integer = 100,
	variable_integer_too_large,
	container_too_long,
	malformed_receive_maximum,
	malformed_maximum_qos,
	bad_property_identifier,

	duplicate_payload_format_indicator = 4000 + 1,
	duplicate_message_expiry_interval = 4000 + 2,
	duplicate_content_type = 4000 + 3,
	duplicate_response_topic = 4000 + 8,
	duplicate_correlation_data = 4000 + 9,
	duplicate_subscription_identifier = 4000 + 11,
	duplicate_session_expiry_interval = 4000 + 17,
	duplicate_assigned_client_identifier = 4000 + 18,
	duplicate_server_keep_alive = 4000 + 19,
	duplicate_authentication_method = 4000 + 21,
	duplicate_authentication_data = 4000 + 22,
	duplicate_request_problem_information = 4000 + 23,
	duplicate_will_delay_interval = 4000 + 24,
	duplicate_request_response_information = 4000 + 25,
	duplicate_response_information = 4000 + 26,
	duplicate_server_reference = 4000 + 28,
	duplicate_reason_string = 4000 + 31,
	duplicate_receive_maximum = 4000 + 33,
	duplicate_topic_alias_maximum = 4000 + 34,
	duplicate_topic_alias = 4000 + 35,
	duplicate_maximum_qos = 4000 + 36,
	duplicate_retain_available = 4000 + 37,
	duplicate_user_property = 4000 + 38,
	duplicate_maximum_packet_size = 4000 + 39,
	duplicate_wildcard_subscription_available = 4000 + 40,
	duplicate_subscription_identifier_available = 4000 + 41,
	duplicate_shared_subscription_available = 4000 + 42,

	// bad_pingresp_flags,
	// no_pingresp_payload_allowed,
	// bad_connack_flags,
	// bad_connack_payload,
	// bad_connack_return_code,
	// bad_puback_flags,
	// bad_puback_payload,
	// bad_pubrec_flags,
	// bad_pubrec_payload,
	// bad_pubrel_flags,
	// bad_pubrel_payload,
	// bad_pubcomp_flags,
	// bad_pubcomp_payload,
	// bad_suback_flags,
	// bad_unsuback_flags,
	// bad_unsuback_payload,
	// bad_packet_type,
	// bad_qos,
	// bad_will,
	// bad_username_password,
	// empty_client_identifier,

	// bad_variant_cast,
	// connection_timed_out,
};

enum class ErrorCondition {
	success,

	protocol_error,
};

inline const std::error_category& terraqtt_error_code_category() noexcept
{
	static class : public std::error_category {
	public:

		const char* name() const noexcept override { return "terraqtt"; }
		std::string message(int e) const override
		{
			switch (static_cast<Error>(e)) {
			// case Error::empty_client_identifier: return "empty client identifier";
			// case Error::bad_variant_cast: return "bad variant cast";
			// case Error::connection_timed_out: return "connection timed out";
			default: return "(unknown error code)";
			}
		}
	} category;
	return category;
}

inline std::error_code make_error_code(Error e) noexcept
{
	return { static_cast<int>(e), terraqtt_error_code_category() };
}

inline const std::error_category& terraqtt_error_condition_category() noexcept
{
	static class : public std::error_category {
	public:
		const char* name() const noexcept override { return "terraqtt"; }
		bool equivalent(const std::error_code& code, int condition) const noexcept override
		{
			if (code.category() != terraqtt_error_code_category()) {
				return false;
			}
			const int value = code.value();
			switch (static_cast<ErrorCondition>(condition)) {
			case ErrorCondition::success: return value == static_cast<int>(Error::success);
			case ErrorCondition::protocol_error: return value >= 100 && value <= 500;
			}
			return false;
		}
		std::string message(int e) const override
		{
			switch (static_cast<ErrorCondition>(e)) {
			case ErrorCondition::success: return "success";
			case ErrorCondition::protocol_error: return "protocol error";
			default: return "(unknown error condition)";
			}
		}
	} category;
	return category;
}

inline std::error_condition make_error_condition(ErrorCondition c) noexcept
{
	return { static_cast<int>(c), terraqtt_error_condition_category() };
}

} // namespace terraqtt

namespace std {

template<>
struct is_error_code_enum<terraqtt::Error> : true_type {};

template<>
struct is_error_condition_enum<terraqtt::ErrorCondition> : true_type {};

} // namespace std
