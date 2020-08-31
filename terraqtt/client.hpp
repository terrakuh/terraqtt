#ifndef TERRAQTT_CLIENT_HPP_
#define TERRAQTT_CLIENT_HPP_

#include "detail/constrained_streambuf.hpp"
#include "keep_aliver.hpp"
#include "lock_guard.hpp"
#include "protocol/connection.hpp"
#include "protocol/ping.hpp"
#include "protocol/publishing.hpp"
#include "protocol/reader.hpp"
#include "protocol/subscription.hpp"
#include "variant.hpp"

#include <initializer_list>
#include <limits>
#include <ratio>

namespace terraqtt {

template<typename Input, typename Output, typename String, typename ReturnCodeContainer,
         typename BasicLockable, typename Clock>
class basic_client
{
public:
	typedef String string_type;
	typedef ReturnCodeContainer return_code_container_type;

	basic_client(Input* input, Output* output) noexcept : _input{ input }, _output{ output }
	{}
	virtual ~basic_client()
	{
		std::error_code ec;
		disconnect(ec);
	}
#if defined(__cpp_exceptions)
	template<typename Identifier>
	void connect(const Identifier& identifier, bool clean_session = true, seconds keep_alive = seconds{ 0 })
	{
		std::error_code ec;

		if (connect(ec, identifier, clean_session, keep_alive), ec) {
			throw exception{ ec };
		}
	}
#endif
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
	void connect(std::error_code& ec, const Identifier& identifier, bool clean_session = true,
	             seconds keep_alive = seconds{ 0 })
	{
		protocol::connect_header<const Identifier&, const String&, const String&> header{ identifier };
		header.clean_session = clean_session;
		header.keep_alive    = keep_alive.count();
		_keep_alive          = { keep_alive };

		lock_guard<BasicLockable> _{ _output_mutex };
		protocol::write_packet(*_output, ec, header);
	}
#if defined(__cpp_exceptions)
	void disconnect()
	{
		std::error_code ec;

		if (disconnect(ec), ec) {
			throw exception{ ec };
		}
	}
#endif
	void disconnect(std::error_code& ec)
	{
		if (_output) {
			lock_guard<BasicLockable> _{ _output_mutex };
			protocol::write_packet(*_output, ec, protocol::disconnect_header{});

			_output = nullptr;
		}
	}
#if defined(__cpp_exceptions)
	template<typename Topic, typename Payload>
	void publish(const Topic& topic, const Payload& payload, qos qos = qos::at_most_once, bool retain = false)
	{
		std::error_code ec;

		if (publish(ec, topic, payload, qos, retain), ec) {
			throw exception{ ec };
		}
	}
#endif
	template<typename Topic, typename Payload>
	void publish(std::error_code& ec, const Topic& topic, const Payload& payload, qos qos = qos::at_most_once,
	             bool retain = false)
	{
		protocol::publish_header<const Topic&> header{ topic };
		header.qos               = qos;
		header.retain            = retain;
		header.packet_identifier = 1;

		lock_guard<BasicLockable> _{ _output_mutex };
		protocol::write_packet(*_output, ec, header, payload);
	}
#if defined(__cpp_exceptions)
	template<typename Topic>
	void subscribe(std::initializer_list<Topic> topics)
	{
		std::error_code ec;

		if (subscribe(ec, topics), ec) {
			throw exception{ ec };
		}
	}
#endif
	/**
	 * Subscribes to one or more topics.
	 *
	 * @param topics the topics
	 * @todo add packet identifier
	 */
	template<typename Topic>
	void subscribe(std::error_code& ec, std::initializer_list<Topic> topics)
	{
		protocol::subscribe_header<const std::initializer_list<Topic>&> header{ topics };
		header.packet_identifier = 1;

		lock_guard<BasicLockable> _{ _output_mutex };
		protocol::write_packet(*_output, ec, header);
	}
#if defined(__cpp_exceptions)
	void ping()
	{
		std::error_code ec;

		if (ping(ec), ec) {
			throw exception{ ec };
		}
	}
#endif
	/**
	 * Sends a ping request to broker.
	 */
	void ping(std::error_code& ec)
	{
		lock_guard<BasicLockable> _{ _output_mutex };
		protocol::write_packet(*_output, ec, protocol::pingreq_header{});
	}
#if defined(__cpp_exceptions)
	void update_state()
	{
		std::error_code ec;

		if (update_state(ec), ec) {
			throw exception{ ec };
		}
	}
#endif
	/**
	 * Updates the keep alive state.
	 *
	 * @todo add timeout
	 */
	void update_state(std::error_code& ec)
	{
		if (_keep_alive.needs_keeping_alive()) {
			if (ping(ec), !ec) {
				_keep_alive.start_reset_timeout();
			}
		} else if (_keep_alive.reset_timeout()) {
			ec = errc::connection_timed_out;
		}
	}
#if defined(__cpp_exceptions)
	std::size_t process_one(std::size_t available = std::numeric_limits<std::size_t>::max())
	{
		std::error_code ec;
		const auto processed = process_one(ec, available);
		return ec ? throw exception{ ec } : processed;
	}
#endif
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
		case protocol::control_packet_type::connack: _handle_connack(ec); break;
		case protocol::control_packet_type::publish: _handle_publish(ec); break;
		case protocol::control_packet_type::puback: _handle_puback(ec); break;
		case protocol::control_packet_type::pubrec: _handle_pubrec(ec); break;
		case protocol::control_packet_type::pubrel: _handle_pubrel(ec); break;
		case protocol::control_packet_type::pubcomp: _handle_pubcomp(ec); break;
		case protocol::control_packet_type::suback: _handle_suback(ec); break;
		case protocol::control_packet_type::unsuback: _handle_unsuback(ec); break;
		case protocol::control_packet_type::pingresp: _handle_pingresp(ec); break;
		default: ec = errc::bad_packet_type; break;
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
	virtual void on_connack(protocol::connack_header& header)
	{
		_keep_alive.reset();
	}
	virtual void on_publish(protocol::publish_header<string_type>& header, std::istream& payload,
	                        std::size_t payload_size)
	{}
	virtual void on_puback(protocol::puback_header& header)
	{
		_keep_alive.reset();
	}
	virtual void on_pubrec(protocol::pubrec_header& header)
	{}
	virtual void on_pubrel(protocol::pubrel_header& header)
	{}
	virtual void on_pubcomp(protocol::pubcomp_header& header)
	{}
	virtual void on_suback(protocol::suback_header<return_code_container_type>& header)
	{
		_keep_alive.reset();
	}
	virtual void on_unsuback(protocol::unsuback_header& header)
	{
		_keep_alive.reset();
	}
	virtual void on_pingresp(protocol::pingresp_header& header)
	{
		_keep_alive.reset();
	}

private:
	std::size_t _read_ignore = 0;
	protocol::read_context _read_context{};
	protocol::control_packet_type _read_type = protocol::control_packet_type::reserved;
	variant<protocol::connack_header, protocol::publish_header<String>, protocol::puback_header,
	        protocol::pubrec_header, protocol::pubrel_header, protocol::pubcomp_header,
	        protocol::suback_header<ReturnCodeContainer>, protocol::unsuback_header,
	        protocol::pingresp_header>
	    _read_header;
	/** the keep alive timeout */
	keep_aliver<Clock> _keep_alive;
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
	void _handle_connack(std::error_code& ec)
	{
		constexpr auto index = 0;

		if (!_read_context.sequence) {
			_read_header.template emplace<index>();
		}

		if (protocol::read_packet(*_input, ec, _read_context, *_read_header.template get<index>(ec)) && !ec) {
			_clear_read();
			on_connack(*_read_header.template get<index>(ec));
		}
	}
	void _handle_publish(std::error_code& ec)
	{
		constexpr auto index = 1;

		if (!_read_context.sequence) {
			_read_header.template emplace<index>();
		}

		protocol::variable_integer_type payload_size = 0;
		const auto total_available                   = _read_context.available;

		if (protocol::read_packet(*_input, ec, _read_context, *_read_header.template get<index>(ec),
		                          payload_size) &&
		    !ec) {
			_clear_read();

			detail::constrained_streambuf buf{ *_input->rdbuf(), payload_size };
			std::istream payload{ &buf };
			on_publish(*_read_header.template get<index>(ec), payload, payload_size);

			// ignore remaining payload
			_read_ignore         = buf.remaining();
			const auto available = total_available - _read_context.available - payload_size + buf.remaining();

			if (const auto skip = _read_ignore < available ? _read_ignore : available) {
				_input->ignore(skip);

				_read_ignore -= skip;
			}
		}
	}
	void _handle_puback(std::error_code& ec)
	{
		constexpr auto index = 2;

		if (!_read_context.sequence) {
			_read_header.template emplace<index>();
		}

		if (protocol::read_packet(*_input, ec, _read_context, *_read_header.template get<index>(ec)) && !ec) {
			_clear_read();
			on_puback(*_read_header.template get<index>(ec));
		}
	}
	void _handle_pubrec(std::error_code& ec)
	{
		constexpr auto index = 3;

		if (!_read_context.sequence) {
			_read_header.template emplace<index>();
		}

		if (protocol::read_packet(*_input, ec, _read_context, *_read_header.template get<index>(ec)) && !ec) {
			_clear_read();
			on_pubrec(*_read_header.template get<index>(ec));
		}
	}
	void _handle_pubrel(std::error_code& ec)
	{
		constexpr auto index = 4;

		if (!_read_context.sequence) {
			_read_header.template emplace<index>();
		}

		if (protocol::read_packet(*_input, ec, _read_context, *_read_header.template get<index>(ec)) && !ec) {
			_clear_read();
			on_pubrel(*_read_header.template get<index>(ec));
		}
	}
	void _handle_pubcomp(std::error_code& ec)
	{
		constexpr auto index = 5;

		if (!_read_context.sequence) {
			_read_header.template emplace<index>();
		}

		if (protocol::read_packet(*_input, ec, _read_context, *_read_header.template get<index>(ec)) && !ec) {
			_clear_read();
			on_pubcomp(*_read_header.template get<index>(ec));
		}
	}
	void _handle_suback(std::error_code& ec)
	{
		constexpr auto index = 6;

		if (!_read_context.sequence) {
			_read_header.template emplace<index>();
		}

		if (protocol::read_packet(*_input, ec, _read_context, *_read_header.template get<index>(ec)) && !ec) {
			_clear_read();
			on_suback(*_read_header.template get<index>(ec));
		}
	}
	void _handle_unsuback(std::error_code& ec)
	{
		constexpr auto index = 7;

		if (!_read_context.sequence) {
			_read_header.template emplace<index>();
		}

		if (protocol::read_packet(*_input, ec, _read_context, *_read_header.template get<index>(ec)) && !ec) {
			_clear_read();
			on_unsuback(*_read_header.template get<index>(ec));
		}
	}
	void _handle_pingresp(std::error_code& ec)
	{
		constexpr auto index = 8;

		if (!_read_context.sequence) {
			_read_header.template emplace<index>();
		}

		if (protocol::read_packet(*_input, ec, _read_context, *_read_header.template get<index>(ec)) && !ec) {
			_clear_read();
			on_pingresp(*_read_header.template get<index>(ec));
		}
	}
};

} // namespace terraqtt

#endif
