#pragma once

#include "../../concepts.hpp"
#include "../../config.hpp"
#include "../../detail/tuple_slice.hpp"
#include "../../error.hpp"
#include "../general.hpp"
#include "concepts.hpp"
#include "variable_integer.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <chrono>
#include <cstring>
#include <optional>
#include <tuple>
#include <type_traits>

namespace terraqtt::protocol::io {

namespace detail {

struct Empty {};

template<typename Type, int Version>
struct ReadableContainerIoContext {
	static std::int32_t make_read_context() noexcept { return -1; }
	static OperationReturn read(std::int32_t& remaining_length, std::span<const std::byte> buffer,
	                            Type& value) noexcept
	{
		std::size_t consumed = 0;
		if (remaining_length == -1) {
			auto read_context = IoContext<std::uint16_t, Version>::make_read_context();
			std::uint16_t size = 0;
			IoContext<std::uint16_t, Version>::read(read_context, buffer.subspan(0, 2), size);
			consumed = 2;
			remaining_length = size;
			value.resize(size);
		}

		const std::size_t size = std::min(value.size() - remaining_length, buffer.size() - consumed);
		std::memcpy(value.data() + value.size() - remaining_length, buffer.data() + consumed, size);
		remaining_length -= size;
		return { .consumed = consumed, .next_call_size = static_cast<std::size_t>(remaining_length) };
	}
};

} // namespace detail

template<typename Type, int Version>
  requires(std::is_integral_v<Type> || std::is_enum_v<Type>)
struct IoContext<Type, Version> {
	constexpr static std::size_t min_size = sizeof(Type);
	constexpr static std::size_t max_request = 0;

	static WriteSizeReturn write_size(Type /* value */) noexcept { return { {}, sizeof(Type) }; }
	static int make_write_context() noexcept { return 0; }
	static OperationReturn write(int& /* context */, std::span<std::byte> buffer, Type value) noexcept
	{
		if constexpr (std::endian::native == std::endian::little) {
			std::reverse_copy(reinterpret_cast<std::byte*>(&value),
			                  reinterpret_cast<std::byte*>(&value) + sizeof(Type), buffer.data());
		} else {
			std::memcpy(buffer.data(), &value, sizeof(Type));
		}
		return { .consumed = sizeof(Type) };
	}
	static int make_read_context() noexcept { return 0; }
	static OperationReturn read(int& /* context */, std::span<const std::byte> buffer, Type& value) noexcept
	{
		if constexpr (std::endian::native == std::endian::little) {
			std::reverse_copy(buffer.begin(), buffer.begin() + sizeof(Type), reinterpret_cast<std::byte*>(&value));
		} else {
			std::memcpy(&value, buffer.data(), sizeof(Type));
		}
		return { .consumed = sizeof(Type) };
	}
};

template<typename Type, typename Period, int Version>
struct IoContext<std::chrono::duration<Type, Period>, Version> {
	using MainContext = IoContext<Type, Version>;

	constexpr static std::size_t min_size = MainContext::min_size;
	constexpr static std::size_t max_request = MainContext::max_request;

	static WriteSizeReturn write_size(std::chrono::duration<Type, Period> value) noexcept
	{
		return MainContext::write_size(value.count());
	}
	static auto make_write_context() noexcept { return MainContext::make_write_context(); }
	static OperationReturn write(decltype(make_write_context())& context, std::span<std::byte> buffer,
	                             std::chrono::duration<Type, Period> value) noexcept
	{
		return MainContext::write(context, buffer, value.count());
	}
	static auto make_read_context() noexcept { return MainContext::make_read_context(); }
	static OperationReturn read(decltype(make_read_context())& context, std::span<const std::byte> buffer,
	                            std::chrono::duration<Type, Period>& value) noexcept
	{
		Type tmp{};
		auto result = MainContext::read(context, buffer, tmp);
		value = std::chrono::duration<Type, Period>{ tmp };
		return result;
	}
};

template<typename Type, int Version>
  requires(ReadableContainer<Type> || ReadableString<Type>)
struct IoContext<Type, Version>
    : std::conditional_t<WriteableContainer<Type> || WriteableString<Type>,
                         detail::ReadableContainerIoContext<Type, Version>, detail::Empty> {
	constexpr static std::size_t min_size = 2;
	constexpr static std::size_t max_request = TERRAQTT_BUFFER_SIZE;

	static WriteSizeReturn write_size(const Type& value) noexcept
	{
		if (value.size() > 65'535) {
			return { Error::container_too_long, 0 };
		}
		return { {}, value.size() + 2 };
	}
	static std::int32_t make_write_context() noexcept { return -1; }
	static OperationReturn write(std::int32_t& already_written, std::span<std::byte> buffer, const Type& value)
	{
		std::size_t consumed = 0;
		if (already_written == -1) {
			if (value.size() > 65'535) {
				return { .ec = Error::container_too_long };
			}
			auto ctx = IoContext<std::uint16_t, Version>::make_write_context();
			IoContext<std::uint16_t, Version>::write(ctx, buffer.subspan(0, 2),
			                                         static_cast<std::uint16_t>(value.size()));
			already_written = 0;
			consumed = 2;
		}

		const std::size_t size = std::min(value.size() - already_written, buffer.size() - consumed);
		std::memcpy(buffer.data() + consumed, value.data(), size);
		already_written += size;
		return { .consumed = consumed + size, .next_call_size = value.size() - already_written };
	}
};

template<typename Type, int Version>
struct IoContext<std::optional<Type>, Version> {
	// static_assert(Writeable<Type, Version>, "Type has no context.");

	constexpr static std::size_t min_size = IoContext<Type, Version>::min_size;
	constexpr static std::size_t max_request = IoContext<Type, Version>::max_request;

	static WriteSizeReturn write_size(const std::optional<Type>& value) noexcept
	{
		if (value.has_value()) {
			return IoContext<Type, Version>::write_size(value.value());
		}
		return { {}, 0 };
	}
	static auto make_write_context() noexcept { return IoContext<Type, Version>::make_write_context(); }
	static OperationReturn write(decltype(make_write_context())& context, std::span<std::byte> buffer,
	                             const std::optional<Type>& value)
	{
		if (value.has_value()) {
			return IoContext<Type, Version>::write(context, buffer, value.value());
		}
		return {};
	}
};

template<typename... Types, int Version>
  requires(sizeof...(Types) > 0)
struct IoContext<std::tuple<Types...>, Version> {
	// static_assert((... && Writeable<Types, Version>), "At least one type has no context.");

	constexpr static std::size_t min_size = (IoContext<Types, Version>::min_size + ...);
	constexpr static std::size_t max_request = std::max({ IoContext<Types, Version>::max_request... });

	static WriteSizeReturn write_size(const std::tuple<Types...>& value) noexcept
	{
		std::error_code ec{};
		std::size_t size = 0;
		std::apply(
		  [&](const auto&... values) {
			  std::size_t tmp = 0;
			  ((ec ? static_cast<void>(0)
			       : static_cast<void>(
			           std::tie(ec, tmp) =
			             IoContext<std::remove_cvref_t<decltype(values)>, Version>::write_size(values),
			           size += tmp)),
			   ...);
		  },
		  value);
		return { ec, size };
	}
	static auto make_write_context() noexcept
	{
		// The first element contains which element is currently is processed.
		return std::make_tuple(0, IoContext<Types, Version>::make_write_context()...);
	}
	static OperationReturn write(decltype(make_write_context())& context, std::span<std::byte> buffer,
	                             const std::tuple<Types...>& value) noexcept
	{
		OperationReturn r{};
		const std::size_t original_size = buffer.size();
		std::apply(
		  // Unpack tuples.
		  [&](const auto&... values) {
			  // Create compile time indices for the values to extract their corresponding contexts.
			  [&]<std::size_t... Indices>(std::index_sequence<Indices...>) {
				  // Only process if the index is the active one.
				  ((Indices == std::get<0>(context) ? [&](const auto& value) {
						using IoContext_ = IoContext<std::remove_cvref_t<decltype(value)>, Version > ;

						if (r.ec) {
							return;
						}

						// Because previous elements could have consumed a lot of bytes from the buffer.
						// Check if the min_size is satisfied for the first write() call.
						if (IoContext_::min_size > buffer.size()) {
							r.next_call_size = IoContext_::min_size;
							return;
						}

						r = IoContext_::write(std::get<Indices + 1>(context), buffer, value);
						buffer = buffer.subspan(r.consumed);

						// This element is done. Move to next.
						if (r.next_call_size == 0) {
							std::get<0>(context)++;
						}
			    }(values) : static_cast<void>(0)),
			   ...);
			  }(std::make_index_sequence<sizeof...(values)>{});
		  },
		  value);
		r.consumed = original_size - buffer.size();
		return r;
	}
};

} // namespace terraqtt::protocol::io
