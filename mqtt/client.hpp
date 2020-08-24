#ifndef MQTT_CLIENT_HPP_
#define MQTT_CLIENT_HPP_

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

namespace mqtt {

typedef std::chrono::duration<std::uint16_t> seconds;

template<typename String, typename ReturnCodeContainer, typename BasicLockable, typename Clock>
class basic_client
{
public:
	typedef String string_type;
	typedef ReturnCodeContainer return_code_container_type;

	basic_client(protocol::byte_ostream& output, protocol::byte_istream& input) noexcept
	    : _output{ &output }, _input{ &input }
	{}
	virtual ~basic_client()
	{
		try {
			disconnect();
		} catch (...) {
		}
	}
	/**
	 * Connects to the MQTT broker.
	 *
	 * @param identifier the identifier of this client; must be present if `clean_session == false`
	 * @param clean_session whether the session should be reset after disconnecting
	 * @todo add will and authentication
	 */
	template<typename Identifier>
	void connect(const Identifier& identifier, bool clean_session = true, seconds keep_alive = seconds{ 0 })
	{
		protocol::connect_header<const Identifier&, const String&, const String&> header{ identifier };

		header.clean_session = clean_session;
		header.keep_alive    = keep_alive.count();
		_keep_alive          = keep_alive;

		lock_guard<BasicLockable> _{ _output_mutex };

		protocol::write_packet(*_output, header);
	}
	void disconnect()
	{
		if (_output) {
			lock_guard<BasicLockable> _{ _output_mutex };

			protocol::write_packet(*_output, protocol::disconnect_header{});

			_output = nullptr;
		}
	}
	template<typename Topic, typename Payload>
	void publish(const Topic& topic, const Payload& payload, qos qos = qos::at_most_once, bool retain = false)
	{
		protocol::publish_header<const Topic&> header{ topic };

		header.qos               = qos;
		header.retain            = retain;
		header.packet_identifier = 1;

		lock_guard<BasicLockable> _{ _output_mutex };

		protocol::write_packet(*_output, header, payload);
	}
	template<typename Topic>
	void subscribe(std::initializer_list<Topic> topics)
	{
		protocol::subscribe_header<const std::initializer_list<Topic>&> header{ topics };

		header.packet_identifier = 1;

		lock_guard<BasicLockable> _{ _output_mutex };

		protocol::write_packet(*_output, header);
	}
	/**
	 * Sends a ping request to broker.
	*/
	void ping()
	{
		lock_guard<BasicLockable> _{ _output_mutex };

		protocol::write_packet(*_output, protocol::pingreq_header{});
	}
	void update_state()
	{
		if (_next_keep_alive <= Clock::now()) {
			puts("pinging");
			ping();
		}
	}
	/**
	 * Processes up to `available` bytes. Can be used in a non-blocking fashion.
	 * 
	 * @param available the amount of bytes available to read
	 * @return the amount of bytes processed; does not include publish packet payload
	*/
	std::size_t process_one(std::size_t available = std::numeric_limits<std::size_t>::max())
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
			_read_type = protocol::peek_type(*_input, _read_context);
		}

		// process type
		switch (_read_type) {
		case protocol::control_packet_type::reserved: break;
		case protocol::control_packet_type::connack: {
			constexpr auto index = 0;

			if (!_read_context.sequence) {
				_read_header.template emplace<index>();
			}

			if (protocol::read_packet(*_input, _read_context, _read_header.template get<index>())) {
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

			if (protocol::read_packet(*_input, _read_context, _read_header.template get<index>(),
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

			if (protocol::read_packet(*_input, _read_context, _read_header.template get<index>())) {
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

			if (protocol::read_packet(*_input, _read_context, _read_header.template get<index>())) {
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

			if (protocol::read_packet(*_input, _read_context, _read_header.template get<index>())) {
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

			if (protocol::read_packet(*_input, _read_context, _read_header.template get<index>())) {
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

			if (protocol::read_packet(*_input, _read_context, _read_header.template get<index>())) {
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

			if (protocol::read_packet(*_input, _read_context, _read_header.template get<index>())) {
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

			if (protocol::read_packet(*_input, _read_context, _read_header.template get<index>())) {
				_clear_read();
				on_pingresp(_read_header.template get<index>());
			}

			break;
		}
		default: throw protocol::protocol_error{ "invalid packet type" };
		}

		if (_read_context.available != available) {
			_next_keep_alive = Clock::now() + _keep_alive;
		}

		return available - _read_context.available;
	}
	protocol::byte_ostream* output() noexcept
	{
		return _output;
	}
	protocol::byte_istream* input() noexcept
	{
		return _input;
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
	typename Clock::time_point _next_keep_alive;
	seconds _keep_alive;
	BasicLockable _output_mutex;
	protocol::byte_ostream* _output;
	protocol::byte_istream* _input;

	void _clear_read() noexcept
	{
		_read_type = protocol::control_packet_type::reserved;

		_read_context.clear();
	}
};

} // namespace mqtt

#endif
