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

#include <algorithm>
#include <initializer_list>
#include <limits>
#include <ratio>

namespace terraqtt {

/// @example lightweight.cpp

/**
 * @code{.cpp}
 * using namespace terraqtt;
 *
 * typedef Basic_client<std::istream, std::ostream, std::string, std::vector<protocol::Suback_return_code>,
 *                      std::mutex, std::chrono::steady_clock> Easy_client;
 * @endcode
 */

/**
 * A basic MQTT client. This class only provides the means to communicate with the broker and does not
 * actually follow the semantics of the protocol like keeping track of packet ids, acknowledging messages or
 * quality of service.
 *
 * @see Null_mutex, Static_container, String_view
 * @tparam Input The input stream type.
 * @tparam Output The output stream type.
 * @tparam String In what container strings like topics should be stored. Must statisfy the
 * [Container](https://en.cppreference.com/w/cpp/named_req/Container) requirements.
 * @tparam Mutex A mutex type used for synchronization. Must satisfy the
 * [BasicLockable](https://en.cppreference.com/w/cpp/named_req/BasicLockable) requirements.
 * @tparam Clock The clock used for timing purposes like keep alive. Must statisfy the
 * [Clock](https://en.cppreference.com/w/cpp/named_req/Clock) requirements.
 */
template<typename Input, typename Output, typename String, typename Return_code_container, typename Mutex,
         typename Clock>
class Basic_client
{
public:
	typedef String String_type;
	typedef Return_code_container Return_code_container_type;

	/**
	 * Constructor.
	 *
	 * @param[in] input The input stream.
	 * @param[in] output The output stream.
	 */
	Basic_client(Input& input, Output& output) noexcept : _input{ &input }, _output{ &output }
	{}
	/// Destructor. Automatically disconnects from the server without waiting for the response.
	virtual ~Basic_client() noexcept
	{
		std::error_code ec;
		disconnect(ec);
	}
#if defined(__cpp_exceptions)
	template<typename Identifier>
	void connect(const Identifier& identifier, bool clean_session = true, Seconds keep_alive = Seconds{ 0 })
	{
		std::error_code ec;
		if (connect(ec, identifier, clean_session, keep_alive), ec) {
			throw std::system_error{ ec };
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
	             Seconds keep_alive = Seconds{ 0 })
	{
		protocol::Connect_header<const Identifier&, const String&, const String&> header{ identifier };
		header.clean_session = clean_session;
		header.keep_alive    = keep_alive.count();
		_keep_alive          = { keep_alive };

		Lock_guard<Mutex> _{ _output_mutex };
		protocol::write_packet(*_output, ec, header);
	}
#if defined(__cpp_exceptions)
	void disconnect()
	{
		std::error_code ec;
		if (disconnect(ec), ec) {
			throw std::system_error{ ec };
		}
	}
#endif
	void disconnect(std::error_code& ec)
	{
		if (_output) {
			Lock_guard<Mutex> _{ _output_mutex };
			protocol::write_packet(*_output, ec, protocol::Disconnect_header{});
			_output = nullptr;
		}
	}
#if defined(__cpp_exceptions)
	template<typename Topic, typename Payload>
	void publish(const Topic& topic, const Payload& payload, std::uint16_t packet_id = 0,
	             QoS qos = QoS::at_most_once, bool retain = false)
	{
		std::error_code ec;
		if (publish(ec, topic, payload, packet_id, qos, retain), ec) {
			throw std::system_error{ ec };
		}
	}
#endif
	/**
	 * Publishes the payload to the topic.
	 *
	 * @tparam Topic the topic type; must meet the requriements of a `Container`
	 * @tparam Paylaod the payload; must meet the requirements of a `Container`
	 * @param ec[out] the error code, if any
	 * @param topic the topic
	 * @param payload the payload
	 * @param packet_id the packet identifier; must be non `0` if `qos != qos::at_most_once`
	 * @param qos the QoS
	 * @param retain whether the payload should be stored on the broker
	 */
	template<typename Topic, typename Payload>
	void publish(std::error_code& ec, const Topic& topic, const Payload& payload, std::uint16_t packet_id = 0,
	             QoS qos = QoS::at_most_once, bool retain = false)
	{
		protocol::Publish_header<const Topic&> header{ topic };
		header.qos               = qos;
		header.retain            = retain;
		header.packet_identifier = packet_id;

		Lock_guard<Mutex> _{ _output_mutex };
		protocol::write_packet(*_output, ec, header, payload);
	}
#if defined(__cpp_exceptions)
	template<typename Topic>
	void subscribe(std::initializer_list<Topic> topics, std::uint16_t packet_id)
	{
		std::error_code ec;
		if (subscribe(ec, topics, packet_id), ec) {
			throw std::system_error{ ec };
		}
	}
#endif
	/**
	 * Subscribes to one or more topics.
	 *
	 * @param ec[out] the error code, if any
	 * @param topics the topics
	 * @param packet_id the packet identifier
	 */
	template<typename Topic>
	void subscribe(std::error_code& ec, std::initializer_list<Topic> topics, std::uint16_t packet_id)
	{
		protocol::Subscribe_header<const std::initializer_list<Topic>&> header{ topics };
		header.packet_identifier = packet_id;

		Lock_guard<Mutex> _{ _output_mutex };
		protocol::write_packet(*_output, ec, header);
	}

#if defined(__cpp_exceptions)
	template<typename Topic>
	void unsubscribe(std::initializer_list<Topic> topics, std::uint16_t packet_id)
	{
		std::error_code ec;
		if (unsubscribe(ec, topics, packet_id), ec) {
			throw std::system_error{ ec };
		}
	}
#endif
	/**
	 * Unsubscribes from one or more topics.
	 *
	 * @param ec[out] the error code, if any
	 * @param topics the topics
	 * @param packet_id the packet identifier
	 */
	template<typename Topic>
	void unsubscribe(std::error_code& ec, std::initializer_list<Topic> topics, std::uint16_t packet_id)
	{
		protocol::Unsubscribe_header<const std::initializer_list<Topic>&> header{ topics };
		header.packet_identifier = packet_id;

		Lock_guard<Mutex> _{ _output_mutex };
		protocol::write_packet(*_output, ec, header);
	}
#if defined(__cpp_exceptions)
	void ping()
	{
		std::error_code ec;
		if (ping(ec), ec) {
			throw std::system_error{ ec };
		}
	}
#endif
	/**
	 * Sends a ping request to broker.
	 *
	 * @param ec[out] the error code, if any
	 */
	void ping(std::error_code& ec)
	{
		Lock_guard<Mutex> _{ _output_mutex };
		protocol::write_packet(*_output, ec, protocol::Pingreq_header{});
	}
#if defined(__cpp_exceptions)
	void update_state()
	{
		std::error_code ec;
		if (update_state(ec), ec) {
			throw std::system_error{ ec };
		}
	}
#endif
	/**
	 * Updates the keep alive state.
	 *
	 * @param ec[out] the error code, if any
	 */
	void update_state(std::error_code& ec)
	{
		if (_keep_alive.needs_ping()) {
			if (ping(ec), !ec) {
				_keep_alive.start_ping_timeout();
			}
		} else if (_keep_alive.timed_out()) {
			ec = Error::connection_timed_out;
		}
	}
#if defined(__cpp_exceptions)
	std::size_t process_one(std::size_t available = std::numeric_limits<std::size_t>::max())
	{
		std::error_code ec;
		const auto processed = process_one(ec, available);
		return ec ? throw std::system_error{ ec } : processed;
	}
#endif
	/**
	 * Processes up to `available` bytes. Can be used in a non-blocking fashion.
	 *
	 * @param[out] ec the error code, if any
	 * @param available the amount of bytes available to read
	 * @return the amount of bytes processed
	 */
	std::size_t process_one(std::error_code& ec,
	                        std::size_t available = std::numeric_limits<std::size_t>::max())
	{
		_read_context.available += available;

		// skip
		if (const auto skip = std::min(_read_ignore, _read_context.available)) {
			_input->ignore(skip);
			_read_ignore -= skip;
			_read_context.available -= skip;
		}

		if (!_read_context.available) {
			return 0;
		} // read new packet
		else if (_read_type == protocol::Control_packet_type::reserved) {
			_read_type = protocol::peek_type(*_input, ec, _read_context);
			if (ec) {
				return 0;
			}
		}

		// process type
		switch (_read_type) {
		case protocol::Control_packet_type::reserved: break;
		case protocol::Control_packet_type::connack: _handle_connack(ec); break;
		case protocol::Control_packet_type::publish: _handle_publish(ec); break;
		case protocol::Control_packet_type::puback: _handle_puback(ec); break;
		case protocol::Control_packet_type::pubrec: _handle_pubrec(ec); break;
		case protocol::Control_packet_type::pubrel: _handle_pubrel(ec); break;
		case protocol::Control_packet_type::pubcomp: _handle_pubcomp(ec); break;
		case protocol::Control_packet_type::suback: _handle_suback(ec); break;
		case protocol::Control_packet_type::unsuback: _handle_unsuback(ec); break;
		case protocol::Control_packet_type::pingresp: _handle_pingresp(ec); break;
		default: ec = Error::bad_packet_type; break;
		}
		return _read_context.available > available ? _read_context.available - available
		                                           : available - _read_context.available;
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
	virtual void on_connack(std::error_code& ec, const protocol::Connack_header& header)
	{}
	/**
	 * Called when the broker published something to a subscribed topic. Quality of serivce is not handled.
	 *
	 * @param[out] ec error code if any
	 * @param header information about the received header
	 * @param[in] payload a stream of the payload; if the data is not read when the function exits, the
	 * remaining data is discarded
	 * @param payload_size size of the payload in bytes
	 */
	virtual void on_publish(std::error_code& ec, const protocol::Publish_header<String_type>& header,
	                        std::istream& payload, std::size_t payload_size)
	{}
	virtual void on_puback(std::error_code& ec, const protocol::Puback_header& header)
	{}
	virtual void on_pubrec(std::error_code& ec, const protocol::Pubrec_header& header)
	{}
	virtual void on_pubrel(std::error_code& ec, const protocol::pubrel_header& header)
	{}
	virtual void on_pubcomp(std::error_code& ec, const protocol::Pubcomp_header& header)
	{}
	virtual void on_suback(std::error_code& ec,
	                       const protocol::Suback_header<Return_code_container_type>& header)
	{}
	virtual void on_unsuback(std::error_code& ec, const protocol::Unsuback_header& header)
	{}
	virtual void on_pingresp(std::error_code& ec, const protocol::Pingresp_header& header)
	{}

private:
	/// How many bytes should be ignored for the next read call.
	std::size_t _read_ignore = 0;
	protocol::Read_context _read_context{};
	protocol::Control_packet_type _read_type = protocol::Control_packet_type::reserved;
	Variant<protocol::Connack_header, protocol::Publish_header<String>, protocol::Puback_header,
	        protocol::Pubrec_header, protocol::pubrel_header, protocol::Pubcomp_header,
	        protocol::Suback_header<Return_code_container>, protocol::Unsuback_header,
	        protocol::Pingresp_header>
	    _read_header;
	Keep_aliver<Clock> _keep_alive;
	Mutex _output_mutex;
	Input* _input;
	Output* _output;

	/// Clears the reading state for the next fresh read.
	void _clear_read() noexcept
	{
		_read_type = protocol::Control_packet_type::reserved;
		_read_context.clear();
		_keep_alive.reset();
	}
	void _handle_connack(std::error_code& ec)
	{
		constexpr auto index = 0;
		if (!_read_context.sequence) {
			_read_header.template emplace<index>();
		}

		if (protocol::read_packet(*_input, ec, _read_context, *_read_header.template get<index>(ec)) && !ec) {
			_clear_read();
			auto& header = *_read_header.template get<index>(ec);
			if (!ec) {
				on_connack(ec, header);
			}
		}
	}
	void _handle_publish(std::error_code& ec)
	{
		constexpr auto index = 1;
		if (!_read_context.sequence) {
			_read_header.template emplace<index>();
		}

		protocol::Variable_integer_type payload_size = 0;
		const auto total_available                   = _read_context.available;
		if (protocol::read_packet(*_input, ec, _read_context, *_read_header.template get<index>(ec),
		                          payload_size) &&
		    !ec) {
			_clear_read();
			detail::Constrained_streambuf buf{ *_input->rdbuf(), payload_size };
			std::istream payload{ &buf };
			auto& header = *_read_header.template get<index>(ec);
			if (!ec) {
				on_publish(ec, header, payload, payload_size);

				// ignore remaining
				_read_ignore = buf.remaining();
				_read_context.available =
				    std::min<std::size_t>(0, _read_context.available - (payload_size - buf.remaining()));
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
			auto& header = *_read_header.template get<index>(ec);
			if (!ec) {
				on_puback(ec, header);
			}
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
			auto& header = *_read_header.template get<index>(ec);
			if (!ec) {
				on_pubrec(ec, header);
			}
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
			auto& header = *_read_header.template get<index>(ec);
			if (!ec) {
				on_pubrel(ec, header);
			}
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
			auto& header = *_read_header.template get<index>(ec);
			if (!ec) {
				on_pubcomp(ec, header);
			}
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
			auto& header = *_read_header.template get<index>(ec);
			if (!ec) {
				on_suback(ec, header);
			}
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
			auto& header = *_read_header.template get<index>(ec);
			if (!ec) {
				on_unsuback(ec, header);
			}
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
			auto& header = *_read_header.template get<index>(ec);
			if (!ec) {
				on_pingresp(ec, header);
			}
		}
	}
};

} // namespace terraqtt

#endif
