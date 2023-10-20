#pragma once

#include "../property.hpp"
#include "fwd.hpp"

#include <limits>
#include <utility>

namespace terraqtt::protocol::io {

template<typename Type>
struct Property {
	bool write;
	PropertyIdentifier identifier;
	const Type& value;
};

template<typename Type, int Version>
struct IoContext<Property<Type>, Version> {
	static_assert(IoContext<Type, Version>::max_request != std::numeric_limits<std::size_t>::max(),
	              "This case is not implemented.");

	constexpr static std::size_t min_size = 0;
	constexpr static std::size_t max_request = IoContext<Type, Version>::max_request + 1;

	static WriteSizeReturn write_size(Property<Type> value) noexcept
	{
		if (value.write) {
			return IoContext<Type, Version>::write_size(value.value);
		}
		return {};
	}
	static auto make_write_context() noexcept
	{
		return std::make_pair(false, IoContext<Type, Version>::make_write_context());
	}
	static OperationReturn write(decltype(make_write_context())& context, std::span<std::byte> buffer,
	                             Property<Type> value) noexcept
	{
		if (value.write) {
			std::size_t consumed = 0;

			// First run.
			if (!context.first) {
				// Because buffer is allowed to be empty for the first call.
				if (buffer.empty()) {
					return { .next_call_size = IoContext<Type, Version>::min_size + 1 };
				}
				buffer[0] = static_cast<std::byte>(value.identifier);
				buffer = buffer.subspan(1);
				++consumed;
				context.first = true;

				if (buffer.size() < IoContext<Type, Version>::min_size) {
					return { .consumed = consumed, .next_call_size = IoContext<Type, Version>::min_size };
				}
			}

			auto r = IoContext<Type, Version>::write(context.second, buffer, value.value);
			r.consumed += consumed;
			return r;
		}
		return {};
	}
};

} // namespace terraqtt::protocol::io
