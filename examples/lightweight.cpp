#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <terraqtt/client.hpp>
#include <terraqtt/null_mutex.hpp>
#include <terraqtt/static_container.hpp>
#include <terraqtt/string_view.hpp>
#include <thread>

using namespace boost::asio;
using namespace terraqtt;

/**
 * Uses the basic input and output streams from the standard library. The string is a static container which
 * allows at most 64 characters (including zero terminating).
 */
using Parent =
    Basic_client<std::istream, std::ostream, Static_container<64, char>,
                 Static_container<1, protocol::Suback_return_code>, Null_mutex, std::chrono::steady_clock>;

class Client : public Parent
{
public:
	using Parent::Parent;

protected:
	void on_publish(std::error_code& ec, const protocol::Publish_header<String_type>& header,
	                std::istream& payload, std::size_t payload_size) override
	{
		std::cout << "received (topic='" << header.topic.begin() << "'): " << payload.rdbuf() << std::endl;

		// the protocol requires an acknoledgment for QoS=1
		if (header.qos == QoS::at_least_once) {
			protocol::write_packet(*output(), ec, protocol::Puback_header{ header.packet_identifier });
		}
	}
};

int main()
try {
	ip::tcp::iostream stream;
	stream.connect(ip::tcp::endpoint{ ip::address::from_string("192.168.178.31"), 1883 });
	if (!stream) {
		std::cerr << "failed to connect to broker\n";
		return 1;
	}

	Client client{ stream, stream };
	client.connect(String_view{ "der.klient" }, false, Seconds{ 30 });
	client.subscribe({ Subscribe_topic<String_view>{ "test", QoS::at_most_once } }, 1);

	while (stream) {
		std::error_code e;
		boost::system::error_code ec;

		if (const auto c = client.process_one(e, stream.rdbuf()->lowest_layer().available(ec))) {
			std::cout << "processed: " << c << " bytes (" << e << ")\n";
		}
		if (ec) {
			std::cerr << "stream error: " << ec << std::endl;
			break;
		}

		client.update_state();
		std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
	}
	std::cerr << "stream broken\n";
} catch (const std::system_error& e) {
	std::cerr << "exception caught (" << e.code() << "): " << e.what() << '\n';
	return 1;
}
