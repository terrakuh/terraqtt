#ifndef MQTT_CLIENT_HPP_
#define MQTT_CLIENT_HPP_

#include "protocol/connection.hpp"
#include "protocol/ping.hpp"
#include "protocol/publishing.hpp"

#include <string>
#include <vector>

namespace mqtt {

template<typename String, typename Blob, typename Password = Blob, typename WillMessage = Blob>
class basic_client
{
public:
	basic_client(protocol::byte_ostream& output) : _output{ &output }
	{}
	~basic_client()
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
		protocol::connect_header<const String&, const WillMessage&, const Password&> header{ identifier };

		header.clean_session = clean_session;
		header.keep_alive    = keep_alive;

		protocol::write_packet(*_output, header);
	}
	void disconnect()
	{
		if (_output) {
			protocol::write_packet(*_output, protocol::disconnect_header{});

			_output = nullptr;
		}
	}
	void publish(const String& topic, const Blob& payload, protocol::qos qos = protocol::qos::at_most_once,
	             bool retain = false)
	{
		protocol::publish_header<const String&, const Blob&> header{ topic, payload };

		header.qos               = qos;
		header.retain            = retain;
		header.packet_identifier = 0;

		protocol::write_packet(*_output, header);
	}
	void ping()
	{
		protocol::write_packet(*_output, protocol::pingreq_header{});
	}

protected:
	virtual void on_data()
	{}

private:
	protocol::byte_ostream* _output;
};

typedef basic_client<std::string, std::vector<protocol::byte>> client;

} // namespace mqtt

#endif
