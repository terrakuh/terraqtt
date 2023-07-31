#pragma once

#include "../../detail/templated_of.hpp"
#include "../../detail/tuple_slice.hpp"
#include "general.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <tuple>

namespace terraqtt::protocol::v5 {

enum class ControlPacketType : std::uint8_t {
	connect                 = 1,
	connect_acknowledge     = 2,
	publish                 = 3,
	publish_acknowledge     = 4,
	publish_received        = 5,
	publish_release         = 6,
	publish_complete        = 7,
	subscribe               = 8,
	subscribe_acknowledge   = 9,
	unsubscribe             = 10,
	unsubscribe_acknowledge = 11,
	ping_request            = 12,
	ping_response           = 13,
	disconnect              = 14,
	auth                    = 15
};

struct FixedHeader {
	ControlPacketType type;
	std::uint8_t flags;
	VariableInteger remaining_length;
};

using PacketIdentifier = std::uint16_t;

inline std::size_t write_size(const auto&... field_commands) noexcept
{
	return (0 + ... + ([](const auto& field) {
		        using type = std::remove_cv_t<std::remove_reference_t<decltype(field)>>;
		        if constexpr (std::is_same_v<type, VariableInteger>) {
			        if (static_cast<std::uint32_t>(field) <= 127) {
				        return 1;
			        } else if (static_cast<std::uint32_t>(field) <= 16'383) {
				        return 2;
			        } else if (static_cast<std::uint32_t>(field) <= 2'097'151) {
				        return 3;
			        }
			        return 4;
		        } else if constexpr (std::is_integral_v<type> || std::is_enum_v<type>) {
			        return sizeof(type);
		        } else if constexpr (ReadableString<type>) {
			        // UTF-8 string is encoded with a 2 byte length prefix.
			        return 2 + field.size();
		        } else if constexpr (detail::TemplatedOf<type, std::optional>) {
			        return field.has_value() ? write_size(field.value()) : 0;
		        } else if constexpr (detail::TemplatedOf<type, std::tuple>) {
			        return std::apply(write_size, field);
		        } else if constexpr (detail::TemplatedOf<type, std::chrono::duration>) {
			        return sizeof(typename type::rep);
		        } else {
			        ([]<bool Flag = false>() { static_assert(Flag, "Unsupported type"); })();
		        }
	        }(field_commands)));
}

inline bool write_field(std::byte*& ptr, const std::byte* end, const auto& field)
{
	using type = std::remove_cv_t<std::remove_reference_t<decltype(field)>>;
	if constexpr (std::is_same_v<type, VariableInteger>) {
		if (end - ptr >= 4) {
			auto value = static_cast<std::underlying_type_t<VariableInteger>>(field);
			for (; true; ++ptr) {
				*ptr = static_cast<std::byte>(value % 128);
				value /= 128;
				if (value == 0) {
					break;
				}
				*ptr = static_cast<std::byte>(static_cast<int>(*ptr) | 128);
			}
			++ptr;
			return true;
		}
	} else if constexpr (std::is_integral_v<type> || std::is_enum_v<type>) {
		if (end - ptr >= static_cast<std::ptrdiff_t>(sizeof(type))) {
			if constexpr (std::endian::native == std::endian::little) {
				std::reverse_copy(reinterpret_cast<const std::byte*>(&field),
				                  reinterpret_cast<const std::byte*>(&field) + sizeof(type), ptr);
			} else {
				std::memcpy(ptr, &field, sizeof(type));
			}
			ptr += sizeof(type);
			return true;
		}
	} else if constexpr (ReadableString<type>) {
		if (end - ptr >= static_cast<std::ptrdiff_t>(field.size() + 2)) {
			write_field(ptr, end, static_cast<std::uint16_t>(field.size()));
			std::memcpy(ptr, field.data(), field.size());
			ptr += field.size();
			return true;
		}
	} else if constexpr (detail::TemplatedOf<type, std::optional>) {
		if (field.has_value()) {
			return write_field(ptr, end, field.value());
		}
		return true;
	} else if constexpr (detail::TemplatedOf<type, std::tuple>) {
		const auto old_ptr = ptr;
		if (std::apply([&](const auto&... fields) { return (... && write_field(ptr, end, fields)); }, field)) {
			return true;
		}
		ptr = old_ptr;
	} else if constexpr (detail::TemplatedOf<type, std::chrono::duration>) {
		return write_field(ptr, end, field.count());
	}
	return false;
}

template<AsyncContext Context, typename Type>
  requires(!detail::TemplatedOf<Type, std::optional>)
inline typename Context::template return_type<std::size_t>
  write_async_field(Context& context, std::span<std::byte> buffer, std::byte*& ptr, const Type& field)
{
	std::error_code ec{};
	std::size_t flush_written = 0;

	// If the field would fit in an empty buffer, flush first and write normally.
	const bool would_fit = write_size(field) <= buffer.size();
	const auto end       = buffer.data() + buffer.size();

	// If it does not fit, write the metadata. Metadata must always fit (because the buffer has a margin),
	// otherwise this is an implementation error.
	if (!would_fit) {
		bool written       = true;
		const auto old_ptr = ptr;

		if constexpr (ReadableString<Type>) {
			written = write_field(ptr, end, static_cast<std::uint16_t>(field.size()));
		} else if constexpr (detail::TemplatedOf<Type, std::tuple>) {
			// Every field except except the last one and the size of the last one must fit into the buffer.
			written = std::apply([&](const auto&... fields) { return (... && write_field(ptr, end, fields)); },
			                     detail::forward_tuple_slice<0, std::tuple_size_v<Type> - 1>(field));
			if (written) {
				written = std::apply(
				  [&](const auto& field) { return write_field(ptr, end, static_cast<std::uint16_t>(field.size())); },
				  detail::forward_tuple_slice<std::tuple_size_v<Type> - 1, 1>(field));
			}
		}

		if (!written) {
			ptr = old_ptr;
			// TODO error
		}
	}

	// Flush old buffer.
	if (buffer.data() != ptr) {
		std::tie(ec, flush_written) = co_await context.write(std::span{ buffer.data(), ptr });
		ptr                         = buffer.data();
	}

	std::size_t field_written = 0;
	if (!ec) {
		if (would_fit) {
			write_field(ptr, end, field);
		} else {
			if constexpr (ReadableString<Type>) {
				std::tie(ec, field_written) =
				  co_await context.write(std::span{ reinterpret_cast<const std::byte*>(field.data()), field.size() });
			}
		}
	}

	co_return context.return_value(ec, flush_written + field_written);
}

template<AsyncContext Context, typename Type>
  requires(detail::TemplatedOf<Type, std::optional>)
inline auto write_async_field(Context& context, std::span<std::byte> buffer, std::byte*& ptr,
                              const Type& field)
{
	assert(field.has_value());
	return write_async_field(context, buffer, ptr, field.value());
}

template<AsyncContext Context>
inline typename Context::template return_type<std::size_t> write_packed_fields(Context& context,
                                                                               const auto&... field_commands)
{
	std::error_code ec{};
	std::size_t total_written = 0;

	std::size_t written         = 0;
	constexpr int buffer_margin = 8;
	std::byte write_buffer[128 + buffer_margin];
	auto ptr = write_buffer;

	// The following code will try to write each field to the internal buffer with `write_field`. If this
	// function fails, `write_async_field` is called which flushes the old buffer and writes the field
	// asynchronously. This probably means that the field is something huge like a string or binary data. The
	// goal is to minimize the asynchronous function calls a much as possible.
	(..., (ec ? static_cast<void>(0 /* an error occurred -> return it */)
	          : static_cast<void>(!write_field(ptr, write_buffer - buffer_margin, field_commands)
	                                ? static_cast<void>(std::tie(ec, written) = co_await write_async_field(
	                                                      context, write_buffer, ptr, field_commands),
	                                                    total_written += written)
	                                : static_cast<void>(0))));

	if (std::span remaining{ write_buffer, ptr }; !ec && !remaining.empty()) {
		std::tie(ec, written) = co_await context.write(remaining);
		total_written += written;
	}

	co_return context.return_value(ec, total_written);
}

template<AsyncContext Context>
inline typename Context::template return_type<std::size_t>
  write_whole_packet(Context& context, ControlPacketType type, std::uint8_t flags,
                     std::uint32_t additional_remaining_length, const auto&... field_commands)
{
	const std::byte first_byte = static_cast<std::byte>(static_cast<int>(type) << 4 | flags);
	const auto remaining_length =
	  static_cast<VariableInteger>(write_size(field_commands...) + additional_remaining_length);

	co_return co_await write_packed_fields(context, first_byte, remaining_length, field_commands...);
}

template<typename Type>
inline std::pair<std::error_code, bool> read_field(std::byte*& ptr, const std::byte* end, Type& field)
{
	const auto old_ptr = ptr;
	if constexpr (std::is_same_v<Type, VariableInteger>) {
		std::underlying_type_t<VariableInteger> result{};

		int multiplier = 1;
		std::uint8_t encoded_byte;
		do {
			if (ptr <= begin) {
				break;
			}
			encoded_byte = *ptr++;
			result += (encoded_byte & 127) * multiplier;
			if (multiplier > 128 * 128 * 128) {
				return { Error::malformed_variable_integer, false };
			}
			multiplier *= 128;
		} while (encoded_byte & 128);

	} else if constexpr (std::is_integral_v<Type> || std::is_enum_v<Type>) {
		if (ptr - begin >= static_cast<std::ptrdiff_t>(sizeof(Type))) {
			ptr -= sizeof(Type);
			if constexpr (std::endian::native == std::endian::little) {
				std::reverse_copy(ptr, ptr + sizeof(Type), reinterpret_cast<std::byte*>(&field));
			} else {
				std::memcpy(&field, ptr, sizeof(Type));
			}
			return { {}, true };
		}
	} else if constexpr (ReadableString<Type>) {
		std::uint16_t length = 0;
		if (read_field(ptr, begin, length) && ptr - begin >= static_cast<std::ptrdiff_t>(length)) {
			ptr -= length;
			field.resize(length);
			std::memcpy(ptr, field.data(), field.size());
			return { {}, true };
		}
	} else if constexpr (detail::TemplatedOf<Type, std::chrono::duration>) {
		typename Type::rep raw{};
		if (read_field(ptr, begin, raw)) {
			field = Type{ raw };
			return { {}, true };
		}
	}
	ptr = old_ptr;
	return { {}, false };
}

template<AsyncContext Context, typename Type>
  requires(!detail::TemplatedOf<Type, std::optional>)
inline typename Context::template return_type<std::size_t>
  read_async_field(Context& context, std::uint32_t& remaining_size, std::span<std::byte> buffer,
                   std::byte*& ptr, std::byte*& ptr_end, Type& field)
{
	// Move in-use bytes to front.
	const auto size = static_cast<std::size_t>(ptr_end - ptr);
	std::memmove(buffer.data(), ptr, size);
	ptr     = buffer.data();
	ptr_end = ptr + size;

	// Read some more.
	std::error_code ec{};
	std::size_t read = 0;

	std::tie(ec, read) = co_await context.read(
	  std::span{ ptr_end, std::min(static_cast<std::size_t>(remaining_size), buffer.size() - size) });
	ptr_end += size;
	if (ec) {
		goto gt_end;
	}

	bool success          = false;
	std::tie(ec, success) = read_field(ptr, ptr_end, field);
	if (!ec && success) {
		co_return
	} else if (!ec) {
		std::uint16_t length = 0;
		read_field(ptr, ptr_end, length);
		field.resize(length);

		const auto size = static_cast<std::size_t>(ptr_end - ptr);
		std::memcpy(field.data(), ptr, size);
		ptr = ptr_end;

		co_await context.read(std::span{ reinterpret_cast<std::byte*>(field.data()) + size, length - size })
	}

gt_end:;
	co_return context.return_value(ec, read);
}

template<AsyncContext Context>
inline typename Context::template return_type<std::size_t> read_packed_fields(Context& context,
                                                                              auto&... field_commands)
{
	std::error_code ec{};
	std::size_t total_read = 0;

	std::size_t read = 0;
	std::byte read_buffer[128];
	auto ptr = read_buffer;

	bool successfully_read = false;
	(...,
	 (ec ? static_cast<void>(0 /* an error occurred -> return it */)
	     : static_cast<void>((std::tie(ec, successfully_read) = read_field(ptr, read_buffer, field_commands),
	                          !ec && !successfully_read)
	                           ? static_cast<void>(std::tie(ec, read) = co_await read_async_field(
	                                                 context, read_buffer, ptr, field_commands),
	                                               total_read += read)
	                           : static_cast<void>(0))));

	co_return context.return_value(ec, total_read);
}

enum class PropertyIdentifier {
	payload_format_indicator = 1,
	message_expiry_interval  = 2,
	content_type             = 3,

	response_topic   = 8,
	correlation_data = 9,

	subscription_identifier = 11,

	session_expiry_interval    = 17,
	assigned_client_identifier = 18,
	server_keep_alive          = 19,

	authentication_method        = 21,
	authentication_data          = 22,
	request_problem_information  = 23,
	will_delay_interval          = 24,
	request_response_information = 25,
	response_information         = 26,

	server_reference = 28,

	reason_string = 31,

	receive_maximum                   = 33,
	topic_alias_maximum               = 34,
	topic_alias                       = 35,
	maximum_qos                       = 36,
	retain_available                  = 37,
	user_property                     = 38,
	maximum_packet_size               = 39,
	wildcard_subscription_available   = 40,
	subscription_identifier_available = 41,
	shared_subscription_available     = 42,
};

enum class ReasonCode : std::uint8_t {
	success                      = 0,
	normal_disconnection         = 0,
	granted_qos_0                = 0,
	granted_qos_1                = 1,
	granted_qos_2                = 2,
	disconnect_with_will_message = 4,
	no_matching_subscribers      = 16,
	no_subscription_existed      = 17,
	continue_authentication      = 24,
	reauthenticate               = 25,

	unspecified_error                 = 128,
	malformed_packet                  = 129,
	protocol_error                    = 130,
	implementation_specific_error     = 131,
	unsupported_protocol_version      = 132,
	invalid_client_identifier         = 133,
	bad_username_password             = 134,
	not_authorized                    = 135,
	server_unavailable                = 136,
	server_busy                       = 137,
	banned                            = 138,
	server_shutting_down              = 139,
	bad_authentication_method         = 140,
	keep_alive_timeout                = 141,
	session_taken_over                = 142,
	invalid_topic_filter              = 143,
	invalid_topic_name                = 144,
	packet_id_in_use                  = 145,
	packet_id_not_found               = 146,
	receive_max_exceeded              = 147,
	invalid_topic_alias               = 148,
	packet_too_large                  = 149,
	message_rate_too_high             = 150,
	quota_exceeded                    = 151,
	administrative_action             = 152,
	invalid_payload_format            = 153,
	retain_not_supported              = 154,
	qos_not_supported                 = 155,
	use_another_server                = 156,
	server_moved                      = 157,
	unsupported_shared_subscriptions  = 158,
	connection_rate_exceeded          = 159,
	max_connect_time                  = 160,
	unsupported_subscription_id       = 161,
	unsupported_wildcard_subscription = 162,
};

} // namespace terraqtt::protocol::v5
