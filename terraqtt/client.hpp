#ifndef TERRAQTT_CLIENT_HPP_
#define TERRAQTT_CLIENT_HPP_

#include "detail/constrained_streambuf.hpp"
#include "lock_guard.hpp"
#include "protocol/connection.hpp"
#include "protocol/ping.hpp"
#include "protocol/publishing.hpp"
#include "protocol/reader.hpp"
#include "protocol/subscription.hpp"
#include "variant.hpp"

#include <chrono>
#include <initializer_list>
#include <limits>
#include <ratio>

namespace terraqtt {

typedef std::chrono::duration<std::uint16_t> seconds;

template<typename Input, typename Output, typename String, typename ReturnCodeContainer,
         typename BasicLockable, typename Clock>
class basic_client
{
public:
	typedef String string_type;
	typedef ReturnCodeContainer return_code_container_type;

	basic_client(Input* input, Output* output) noexcept : _output{ output }, _input{ input }
	{}
	virtual ~basic_client()
	{
		disconnect();
	}
	/**
	 * Connects to the MQTT broker.
	 *
	 * @param identifier the identifier of this client; must be present if `clean_session == false`
	 * @param clean_session whether the session should be reset after disconnecting
	 * @param keep_alive after what timeout the broker should think the client is offline; `0` means no
	 * timeout
	 * @todo add will and authentication
	 */
	template<typename Identifier>
	std::error_code connect(const Identifier& identifier, bool clean_session = true,
	                        seconds keep_alive = seconds{ 0 })
	{
		protocol::connect_header<const Identifier&, const String&, const String&> header{ identifier };
		header.clean_session = clean_session;
		header.keep_alive    = keep_alive.count();
		_keep_alive          = keep_alive;

		lock_guard<BasicLockable> _{ _output_mutex };
		std::error_code ec;
		protocol::write_packet(*_output, ec, header);
		return ec;
	}
	std::error_code disconnect()
	{
		if (_output) {
			lock_guard<BasicLockable> _{ _output_mutex };
			std::error_code ec;
			protocol::write_packet(*_output, ec, protocol::disconnect_header{});

			_output = nullptr;

			return ec;
		}

		return {};
	}
	template<typename Topic, typename Payload>
	std::error_code publish(const Topic& topic, const Payload& payload, qos qos = qos::at_most_once,
	                        bool retain = false)
	{
		protocol::publish_header<const Topic&> header{ topic };
		header.qos               = qos;
		header.retain            = retain;
		header.packet_identifier = 1;

		lock_guard<BasicLockable> _{ _output_mutex };
		std::error_code ec;
		protocol::write_packet(*_output, ec, header, payload);
		return ec;
	}
	/**
	 * Subscribes to one or more topics.
	 *
	 * @param topics the topics
	 * @todo add packet identifier
	 */
	template<typename Topic>
	std::error_code subscribe(std::initializer_list<Topic> topics)
	{
		protocol::subscribe_header<const std::initializer_list<Topic>&> header{ topics };
		header.packet_identifier = 1;

		lock_guard<BasicLockable> _{ _output_mutex };
		std::error_code ec;
		protocol::write_packet(*_output, ec, header);
		return ec;
	}
	/**
	 * Sends a ping request to broker.
	 */
	std::error_code ping()
	{
		lock_guard<BasicLockable> _{ _output_mutex };
		std::error_code ec;
		protocol::write_packet(*_output, ec, protocol::pingreq_header{});
		return ec;
	}
	/**
	 * Updates the keep alive state.
	 *
	 * @todo add timeout
	 */
	std::error_code update_state()
	{
		if (_next_keep_alive <= Clock::now()) {
			return ping();
		}

		return {};
	}
	/**
	 * Processes up to `available` bytes. Can be used in a non-blocking fashion.
	 *
	 * @param available the amount of bytes available to read
	 * @return the amount of bytes processed; does not include publish packet payload
	 */
	std::size_t process_one(std::error_code& ec,
	                        std::size_t available = std::numeric_limits<std::size_t>::max())
	{
		if (const auto skip = _read_ignore < available ? _read_ignore : available) {
			_input->ignore(skip);

			_read_ignore -= skip;
			available -= skip;
		}

		if (!(_read_context.available = available)) {
			return 0;
		} // read new packet
		else if (_read_type == protocol::control_packet_type::reserved) {
			_read_type = protocol::peek_type(*_input, ec, _read_context);

			if (ec) {
				return 0;
			}
		}

		// process type
		switch (_read_type) {
		case protocol::control_packet_type::reserved: break;
		case protocol::control_packet_type::connack: {
			constexpr auto index = 0;

			if (!_read_context.sequence) {
				_read_header.template emplace<index>();
			}

			if (protocol::read_packet(*_input, ec, _read_context, _read_header.template get<index>())) {
				_clear_read();
			}

			break;
		}
		case protocol::control_packet_type::publish: {
			constexpr auto index = 1;

			if (!_read_context.sequence) {
				_read_header.template emplace<index>();
			}

			protocol::variable_integer_type payload_size = 0;

			if (protocol::read_packet(*_input, ec, _read_context, _read_header.template get<index>(),
			                          payload_size)) {
				_clear_read();

				detail::constrained_streambuf buf{ *_input->rdbuf(), payload_size };
				std::istream payload{ &buf };
				on_publish(_read_header.template get<index>(), payload, payload_size);
				_read_ignore  = buf.remaining();
				const auto av = available - _read_context.available - payload_size + buf.remaining();

				if (const auto skip = _read_ignore < av ? _read_ignore : av) {
					_input->ignore(skip);

					_read_ignore -= skip;
				}
			}

			break;
		}
		case protocol::control_packet_type::puback: {
			constexpr auto index = 2;

			if (!_read_context.sequence) {
				_read_header.template emplace<index>();
			}

			if (protocol::read_packet(*_input, ec, _read_context, _read_header.template get<index>())) {
				_clear_read();
				on_puback(_read_header.template get<index>());
			}

			break;
		}
		case protocol::control_packet_type::pubrec: {
			constexpr auto index = 3;

			if (!_read_context.sequence) {
				_read_header.template emplace<index>();
			}

			if (protocol::read_packet(*_input, ec, _read_context, _read_header.template get<index>())) {
				_clear_read();
				on_pubrec(_read_header.template get<index>());
			}

			break;
		}
		case protocol::control_packet_type::pubrel: {
			constexpr auto index = 4;

			if (!_read_context.sequence) {
				_read_header.template emplace<index>();
			}

			if (protocol::read_packet(*_input, ec, _read_context, _read_header.template get<index>())) {
				_clear_read();
				on_pubrel(_read_header.template get<index>());
			}

			break;
		}
		case protocol::control_packet_type::pubcomp: {
			constexpr auto index = 5;

			if (!_read_context.sequence) {
				_read_header.template emplace<index>();
			}

			if (protocol::read_packet(*_input, ec, _read_context, _read_header.template get<index>())) {
				_clear_read();
				on_pubcomp(_read_header.template get<index>());
			}

			break;
		}
		case protocol::control_packet_type::suback: {
			constexpr auto index = 6;

			if (!_read_context.sequence) {
				_read_header.template emplace<index>();
			}

			if (protocol::read_packet(*_input, ec, _read_context, _read_header.template get<index>())) {
				_clear_read();
				on_suback(_read_header.template get<index>());
			}

			break;
		}
		case protocol::control_packet_type::unsuback: {
			constexpr auto index = 7;

			if (!_read_context.sequence) {
				_read_header.template emplace<index>();
			}

			if (protocol::read_packet(*_input, ec, _read_context, _read_header.template get<index>())) {
				_clear_read();
				on_unsuback(_read_header.template get<index>());
			}

			break;
		}
		case protocol::control_packet_type::pingresp: {
			constexpr auto index = 8;

			if (!_read_context.sequence) {
				_read_header.template emplace<index>();
			}

			if (protocol::read_packet(*_input, ec, _read_context, _read_header.template get<index>())) {
				_clear_read();
				on_pingresp(_read_header.template get<index>());
			}

			break;
		}
		default: ec = errc::bad_packet_type;
		}

		if (_read_context.available != available) {
			_next_keep_alive = Clock::now() + _keep_alive;
		}

		return available - _read_context.available;
	}
	Input* input() noexcept
	{
		return _input;
	}
	Output* output() noexcept
	{
		return _output;
	}

protected:
	virtual void on_publish(protocol::publish_header<string_type>& header, std::istream& payload,
	                        std::size_t payload_size)
	{}
	virtual void on_puback(protocol::puback_header& header)
	{}
	virtual void on_pubrec(protocol::pubrec_header& header)
	{}
	virtual void on_pubrel(protocol::pubrel_header& header)
	{}
	virtual void on_pubcomp(protocol::pubcomp_header& header)
	{}
	virtual void on_suback(protocol::suback_header<return_code_container_type>& header)
	{}
	virtual void on_unsuback(protocol::unsuback_header& header)
	{}
	virtual void on_pingresp(protocol::pingresp_header& header)
	{}

private:
	std::size_t _read_ignore = 0;
	protocol::read_context _read_context{};
	protocol::control_packet_type _read_type = protocol::control_packet_type::reserved;
	variant<protocol::connack_header, protocol::publish_header<String>, protocol::puback_header,
	        protocol::pubrec_header, protocol::pubrel_header, protocol::pubcomp_header,
	        protocol::suback_header<ReturnCodeContainer>, protocol::unsuback_header,
	        protocol::pingresp_header>
	    _read_header;
	/** describes when the next keep alive action is due */
	typename Clock::time_point _next_keep_alive = Clock::time_point::max();
	/** the keep alive timeout */
	seconds _keep_alive;
	BasicLockable _output_mutex;
	/** the input stream */
	Input* _input;
	/** the output stream */
	Output* _output;

	/**
	 * Clears the reading state for the next fresh read.
	 */
	void _clear_read() noexcept
	{
		_read_type = protocol::control_packet_type::reserved;
		_read_context.clear();
	}
};

} // namespace terraqtt

#endif
