#ifndef MQTT_CLIENT_HPP_
#define MQTT_CLIENT_HPP_

#include "protocol/connection.hpp"
#include "protocol/ping.hpp"
#include "protocol/publishing.hpp"
#include "protocol/reader.hpp"
#include "protocol/subscription.hpp"
#include "variant.hpp"

#include <limits>
#include <mutex>
#include <string>
#include <vector>

namespace mqtt {

template<typename String, typename BasicLockable>
class basic_client_base
{
public:
	typedef String string_type;

	basic_client_base(protocol::byte_ostream& output, protocol::byte_istream& input)
	    : _output{ &output }, _input{ &input }
	{}
	virtual ~basic_client_base()
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
	 */
	void connect(const String& identifier, bool clean_session = true, std::uint16_t keep_alive = 60)
	{
		protocol::connect_header<const String&, const String&, const String&> header{ identifier };

		header.clean_session = clean_session;
		header.keep_alive    = keep_alive;

		std::lock_guard<BasicLockable> _{ _output_mutex };

		protocol::write_packet(*_output, header);
	}
	void disconnect()
	{
		if (_output) {
			std::lock_guard<BasicLockable> _{ _output_mutex };

			protocol::write_packet(*_output, protocol::disconnect_header{});

			_output = nullptr;
		}
	}
	template<typename Topic, typename Payload>
	void publish(const Topic& topic, const Payload& payload, protocol::qos qos = protocol::qos::at_most_once,
	             bool retain = false)
	{
		protocol::publish_header<const Topic&> header{ topic };

		header.qos               = qos;
		header.retain            = retain;
		header.packet_identifier = 0;

		std::lock_guard<BasicLockable> _{ _output_mutex };

		protocol::write_packet(*_output, header, payload);
	}
	void subscribe()
	{
		protocol::subscribe_header<const String&, int> header{};

		// header.packet_identifier =

		std::lock_guard<BasicLockable> _{ _output_mutex };

		protocol::write_packet(*_output, header);
	}
	void ping()
	{
		std::lock_guard<BasicLockable> _{ _output_mutex };

		protocol::write_packet(*_output, protocol::pingreq_header{});
	}
	std::size_t process_one(std::size_t available = std::numeric_limits<std::size_t>::max())
	{
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
				on_publish(_read_header.template get<index>(), payload_size);
			}

			break;
		}
		// case protocol::control_packet_type::puback:
		// case protocol::control_packet_type::pubrec:
		// case protocol::control_packet_type::pubrel:
		// case protocol::control_packet_type::pubcomp:
		// case protocol::control_packet_type::suback:
		case protocol::control_packet_type::unsuback: {
			constexpr auto index = 2;

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
			constexpr auto index = 3;

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

		return available - _read_context.available;
	}

protected:
	virtual void on_publish(protocol::publish_header<String>& header,
	                        protocol::variable_integer_type payload_size)
	{
		std::string s;
		s.resize(payload_size);
		_input->read(&s[0], s.size());
		puts(s.c_str());
	}
	virtual void on_pingresp(protocol::pingresp_header& header)
	{
		puts("ping received");
	}
	virtual void on_unsuback(protocol::unsuback_header& header)
	{}

private:
	protocol::read_context _read_context{};
	protocol::control_packet_type _read_type = protocol::control_packet_type::reserved;
	variant<protocol::connack_header, protocol::publish_header<String>, protocol::unsuback_header,
	        protocol::pingresp_header>
	    _read_header;
	BasicLockable _output_mutex;
	protocol::byte_ostream* _output;
	protocol::byte_istream* _input;

	void _clear_read() noexcept
	{
		_read_type = protocol::control_packet_type::reserved;

		_read_context.clear();
	}
};

template<typename String>
class basic_client : public basic_client_base<String, std::mutex>
{
public:
	using basic_client_base<String, std::mutex>::basic_client_base;
};

typedef basic_client<std::string> client;

} // namespace mqtt

#endif
