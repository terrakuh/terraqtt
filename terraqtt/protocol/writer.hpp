#ifndef TERRAQTT_PROTOCOL_WRITER_HPP_
#define TERRAQTT_PROTOCOL_WRITER_HPP_

#include "general.hpp"

#include <algorithm>
#include <iterator>

namespace terraqtt {
namespace protocol {

/**
 * Writes a byte to the output buffer.
 *
 * @param output[in] the output buffer
 * @param value the value
 * @return the size written to the buffer
 */
inline std::size_t write_element(byte* output, byte value) noexcept
{
	*output = value;

	return 1;
}

/**
 * Writes an uint16 to the output buffer.
 *
 * @param output[in] the output buffer
 * @param value the value
 * @return the size written to the buffer
 */
inline std::size_t write_element(byte* output, std::uint16_t value) noexcept
{
	output[0] = static_cast<byte>(value >> 8);
	output[1] = static_cast<byte>(value & 0xff);

	return 2;
}

/**
 * Writes a variable uint to the output buffer.
 *
 * @exception protocol_error if `value >= 268435456`
 * @param output[in] the output buffer
 * @param value the value
 * @return the size written to the buffer
 */
inline std::size_t write_element(byte* output, variable_integer value)
{
	const auto v = static_cast<typename std::underlying_type<variable_integer>::type>(value);

	if (v < 128) {
		*output = static_cast<byte>(v);

		return 1;
	} else if (v < 16384) {
		output[0] = static_cast<byte>(v >> 7 | 0x80);
		output[1] = static_cast<byte>(v & 0x7f);

		return 2;
	} else if (v < 2097152) {
		output[0] = static_cast<byte>(v >> 14 & 0x7f | 0x80);
		output[1] = static_cast<byte>(v >> 7 & 0x7f | 0x80);
		output[2] = static_cast<byte>(v & 0x7f);

		return 3;
	} else if (v < 268435456) {
		output[0] = static_cast<byte>(v >> 21 & 0x7f | 0x80);
		output[1] = static_cast<byte>(v >> 14 & 0x7f | 0x80);
		output[2] = static_cast<byte>(v >> 7 & 0x7f | 0x80);
		output[3] = static_cast<byte>(v & 0x7f);

		return 4;
	}

	throw protocol_error{ "variable size is too large" };
}

template<typename Element>
inline byte* write_elements(byte* output, Element&& element)
{
	return output + write_element(output, std::forward<Element>(element));
}

template<typename Element, typename... Elements>
inline byte* write_elements(byte* output, Element&& element, Elements&&... elements)
{
	return write_elements(output + write_element(output, std::forward<Element>(element)),
	                      std::forward<Elements>(elements)...);
}

template<typename Output, typename... Elements>
inline void write_elements(Output& output, Elements&&... elements)
{
	byte buffer[elements_max_size<Elements...>()];

	output.write(
	    reinterpret_cast<const typename Output::char_type*>(buffer),
	    static_cast<std::size_t>(write_elements(buffer, std::forward<Elements>(elements)...) - buffer));

	if (!output) {
		throw io_error{ "failed to write elements" };
	}
}

/**
 * Writes a blob and an optional size header to the stream.
 *
 * @exception protocol_error if the string is too large
 * @exception io_error if the string could not be written to the stream
 * @exception see write_elements()
 * @param output[in] the output stream
 * @param blob the blob
 * @tparam WriteSize if `true` writes the size of the blob as uint16
 * @tparam Blob must meet the requirements of *Container* and the size of its type must match
 * `sizeof(Output::char_type)`
 */
template<bool WriteSize, typename Output, typename Blob>
inline void write_blob(Output& output, const Blob& blob)
{
	static_assert(sizeof(typename Blob::value_type) == sizeof(typename Output::char_type),
	              "sizeof blob and output types must match");

	if (WriteSize) {
		const auto size = blob.size();

		if (size > std::numeric_limits<std::uint16_t>::max()) {
			throw protocol_error{ "max string length exceeded" };
		}

		write_elements(output, static_cast<std::uint16_t>(size));
	}

	for (auto i : blob) {
		output.write(reinterpret_cast<const typename Output::char_type*>(&i), sizeof(i));

		if (!output) {
			throw io_error{ "failed to write to the stream" };
		}
	}
}

} // namespace protocol
} // namespace mqtt

#endif
