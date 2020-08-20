#ifndef MQTT_CLIENT_HPP_
#define MQTT_CLIENT_HPP_

#include "protocol/connection.hpp"

namespace mqtt {

class client
{
public:
	client(protocol::byte_ostream& output) : _output{ &output }
	{
		protocol::connect_header header{};

		protocol::write_packet(*_output, header);
	}
	~client()
	{
		try {
			disconnect();
		} catch (...) {
		}
	}
	void disconnect()
	{
		if (_output) {
			protocol::write_packet(*_output, protocol::disconnect_header{});

			_output = nullptr;
		}
	}

protected:
	virtual void on_data()
	{}

private:
	protocol::byte_ostream* _output;
};

} // namespace mqtt

#endif
