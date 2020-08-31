#pragma once

#include <system_error>
#include <type_traits>

namespace terraqtt {

enum class errc
{
	variable_integer_too_large = 1,
	payload_too_large,
	string_too_long,
	bad_remaining_size,

	bad_pingresp_flags,
	no_pingresp_payload_allowed,
	bad_connack_flags,
	bad_connack_payload,
	bad_connack_return_code,
	bad_puback_flags,
	bad_puback_payload,
	bad_pubrec_flags,
	bad_pubrec_payload,
	bad_pubrel_flags,
	bad_pubrel_payload,
	bad_pubcomp_flags,
	bad_pubcomp_payload,
	bad_suback_flags,
	bad_unsuback_flags,
	bad_unsuback_payload,
	bad_packet_type,
	bad_qos,
	bad_will,
	bad_username_password,
	empty_client_identifier,

	bad_variant_cast,
	connection_timed_out,
};

#if defined(__cpp_exceptions)
class exception : public std::system_error
{
public:
	using system_error::system_error;
};
#endif

inline const std::error_category& terraqtt_category() noexcept
{
	static class : public std::error_category
	{
	public:
		const char* name() const noexcept override
		{
			return "terraqtt";
		}
		std::string message(int ec) const override
		{
			switch (static_cast<errc>(ec)) {
			default: return "(unknown error code)";
			}
		}
	} category;

	return category;
}

inline std::error_code make_error_code(errc ec) noexcept
{
	return { static_cast<int>(ec), terraqtt_category() };
}

} // namespace terraqtt

namespace std {

template<>
struct is_error_code_enum<terraqtt::errc> : true_type
{};

} // namespace std
