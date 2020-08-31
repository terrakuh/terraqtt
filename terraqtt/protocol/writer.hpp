#ifndef TERRAQTT_PROTOCOL_WRITER_HPP_
#define TERRAQTT_PROTOCOL_WRITER_HPP_

#include "../error.hpp"
#include "general.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

namespace terraqtt {
namespace protocol {

/**
 * Writes a byte to the output buffer.
 *
 * @param output[in] the output buffer
 * @param ec[out] the output error; not used
 * @param value the value
 * @return the end of the output buffer
 */
inline byte* write_elements(byte* output, std::error_code& ec, byte value) noexcept
{
	*output = value;

	return output + 1;
}

/**
 * Writes an uint16 to the output buffer.
 *
 * @param output[in] the output buffer
 * @param ec[out] the output error; not used
 * @param value the value
 * @return the end of the output buffer
 */
inline byte* write_elements(byte* output, std::error_code& ec, std::uint16_t value) noexcept
{
	output[0] = static_cast<byte>(value >> 8);
	output[1] = static_cast<byte>(value & 0xff);

	return output + 2;
}

/**
 * Writes a variable uint to the output buffer.
 *
 * @param output[in] the output buffer
 * @param ec[out] the output error, if `value >= 268435456`
 * @param value the value
 * @return the end of the output buffer
 */
inline byte* write_elements(byte* output, std::error_code& ec, variable_integer value) noexcept
{
	const auto v = static_cast<typename std::underlying_type<variable_integer>::type>(value);

	if (v < 128) {
		*output = static_cast<byte>(v);

		return output + 1;
	} else if (v < 16384) {
		output[0] = static_cast<byte>(v >> 7 | 0x80);
		output[1] = static_cast<byte>(v & 0x7f);

		return output + 2;
	} else if (v < 2097152) {
		output[0] = static_cast<byte>((v >> 14 & 0x7f) | 0x80);
		output[1] = static_cast<byte>((v >> 7 & 0x7f) | 0x80);
		output[2] = static_cast<byte>(v & 0x7f);

		return output + 3;
	} else if (v < 268435456) {
		output[0] = static_cast<byte>((v >> 21 & 0x7f) | 0x80);
		output[1] = static_cast<byte>((v >> 14 & 0x7f) | 0x80);
		output[2] = static_cast<byte>((v >> 7 & 0x7f) | 0x80);
		output[3] = static_cast<byte>(v & 0x7f);

		return output + 4;
	}

	ec = errc::variable_integer_too_large;

	return output;
}

template<typename Element, typename... Elements>
inline byte* write_elements(byte* output, std::error_code& ec, Element&& element,
                            Elements&&... elements) noexcept
{
	output = write_elements(output, ec, std::forward<Element>(element));

	return ec ? output : write_elements(output, ec, std::forward<Elements>(elements)...);
}

template<typename Output, typename... Elements>
inline void write_elements(Output& output, std::error_code& ec, Elements&&... elements)
{
	byte buffer[elements_max_size<Elements...>()];

	const auto end = write_elements(buffer, ec, std::forward<Elements>(elements)...);

	if (!ec) {
		output.write(reinterpret_cast<const typename Output::char_type*>(buffer),
		             static_cast<std::size_t>(end - buffer));

		if (!output) {
			ec = std::make_error_code(std::errc::io_error);
		}
	}
}

/**
 * Writes a blob and an optional size header to the stream.
 *
 * @exception see write_elements()
 * @param output[in] the output stream
 * @param blob the blob
 * @tparam WriteSize if `true` writes the size of the blob as uint16
 * @tparam Blob must meet the requirements of *Container* and the size of its type must match
 * `sizeof(Output::char_type)`
 */
template<bool WriteSize, typename Output, typename Blob>
inline void write_blob(Output& output, std::error_code& ec, const Blob& blob)
{
	static_assert(sizeof(typename Blob::value_type) == sizeof(typename Output::char_type),
	              "sizeof blob and output types must match");

	if (WriteSize) {
		const auto size = blob.size();

		if (size > std::numeric_limits<std::uint16_t>::max()) {
			ec = errc::string_too_long;
			return;
		}

		write_elements(output, ec, static_cast<std::uint16_t>(size));

		if (ec) {
			return;
		}
	}

	for (auto i : blob) {
		output.write(reinterpret_cast<const typename Output::char_type*>(&i), sizeof(i));

		if (!output) {
			ec = std::make_error_code(std::errc::io_error);
			return;
		}
	}
}

} // namespace protocol
} // namespace terraqtt

#endif
