#pragma once

#include "connect.hpp"
#include "fixed_header.hpp"
#include "general.hpp"

#include <optional>

namespace terraqtt::protocol::v5 {

template<String StringContainer>
struct ConnectAcknowledgeHeader {
	bool session_present;
	ReasonCode reason_code;
	/// If null, the session expiry specified in the connect header counts.
	std::optional<SessionExpiryInterval> session_expiry;
	std::uint16_t receive_maximum = 65'535;
	QualityOfService maximum_qos  = QualityOfService::exactly_once;
	bool retain_available         = true;
	std::optional<StringContainer> assigned_client_identifier;
	std::uint16_t topic_alias_maximum = 0;

};

template<AsyncContext Context, typename StringContainer>
inline typename Context::template return_type<void> read(Context& context,
                                                         ConnectAcknowledgeHeader<StringContainer>& header)
{
	std::error_code ec{};
	std::size_t read = 0;

	std::uint8_t flags = 0;

	std::tie(ec, read) = co_await read_fields(context, flags, header.reason_code);
	if (ec) {
		co_return context.return_value(ec);
	}
	header.session_present = static_cast<bool>(flags & 1);

	// Read all properties.
	VariableInteger property_length{};
	std::tie(ec, property_length) = co_await read_variable_integer(context);

	std::byte buffer[64];
	std::tie(ec, read) = co_await context.read(
	  std::span{ buffer, std::min(sizeof(buffer), static_cast<std::size_t>(property_length)) });

	auto ptr = buffer;
	if (static_cast<PropertyIdentifier>(*ptr) == PropertyIdentifier::session_expiry_interval) {
		SessionExpiryInterval::rep value{};
		ptr                   = read_field(ptr, value);
		header.session_expiry = SessionExpiryInterval{ value };
	}

	if (static_cast<PropertyIdentifier>(*ptr) == PropertyIdentifier::receive_maximum) {
		ptr = read_field(ptr, header.receive_maximum);
		if (header.receive_maximum == 0) {
			ec = Error::malformed_receive_maximum;
		}
	}

	if (static_cast<PropertyIdentifier>(*ptr) == PropertyIdentifier::maximum_qos) {
		ptr = read_field(ptr, header.maximum_qos);
		if (static_cast<int>(header.maximums_qos) > 1) {
			ec = Error::malformed_maximum_qos;
		}
	}

	if (static_cast<PropertyIdentifier>(*ptr) == PropertyIdentifier::retain_available) {
		ptr = read_field(ptr, header.retain_available);
	}

	if (static_cast<PropertyIdentifier>(*ptr) == PropertyIdentifier::assigned_client_identifier) {
		StringContainer string{};
		std::tie(ec, string) = co_await read_string(context, std::span<const std::byte>{ ptr, buffer + read });
		header.assigned_client_identifier = std::move(string);
	}

	if (ec) {
		co_return context.return_value(ec);
	}

	co_return context.return_value(ec);
}

} // namespace terraqtt::protocol::v5
