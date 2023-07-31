#pragma once

#include "fixed_header.hpp"

namespace terraqtt::protocol::v5 {

struct PingRequestHeader {};

template<AsyncContext Context>
inline typename Context::template return_type<std::size_t> write(Context& context, PingRequestHeader)
{
	co_return co_await write_whole_packet(context, ControlPacketType::ping_request, 0, 0);
}

struct PingResponseHeader {};

template<AsyncContext Context>
inline typename Context::template return_type<std::size_t> write(Context& context, PingResponseHeader)
{
	co_return co_await write_whole_packet(context, ControlPacketType::ping_response, 0, 0);
}

} // namespace terraqtt::protocol::v5
