#include <boost/asio.hpp>
#include <boost/asio/detail/socket_option.hpp>
#include <iostream>
#include <string>
#include <terraqtt/client.hpp>
#include <terraqtt/static_container.hpp>
#include <terraqtt/string_view.hpp>
#include <thread>

using namespace boost::asio;
using namespace terraqtt;

/**
 * Uses the basic input and output streams from the standard library. The string is a static container which
 * allows at most 64 characters (including zero terminating).
 */
using Parent = Basic_client<std::istream, std::ostream, Static_container<64, char>,
                            Static_container<1, protocol::Suback_return_code>, std::chrono::steady_clock>;

class Client : public Parent
{
public:
	using Parent::Parent;

protected:
	void on_publish(std::error_code& ec, const protocol::Publish_header<String_type>& header,
	                std::istream& payload, std::size_t payload_size) override
	{
		publish(ec, String_view{ "log" }, String_view{ "some message received" });

		std::cout << "received (topic='";
		std::cout.write(header.topic.begin(), header.topic.size());
		std::cout << "'): " << payload.rdbuf() << std::endl;

		// the protocol requires an acknoledgment for QoS=1
		if (header.qos == QoS::at_least_once) {
			protocol::write_packet(*output(), ec, protocol::Puback_header{ header.packet_identifier });
		}
	}
};

int main()
try {
	spdlog::set_level(spdlog::level::trace);
	ip::tcp::iostream stream;
	stream.connect(ip::tcp::endpoint{ ip::address::from_string("127.0.0.1"), 1883 });
	if (!stream) {
		std::cerr << "Failed to connect to broker\n";
		return 1;
	}

	Client client{ stream, stream };
	client.connect(String_view{ "my-client" }, true, Seconds{ 5 });
	client.subscribe({ Subscribe_topic<String_view>{ "test", QoS::at_most_once } }, 1);
	client.subscribe({ Subscribe_topic<String_view>{ "pc/led/version", QoS::at_most_once } }, 2);
	client.subscribe({ Subscribe_topic<String_view>{ "bed/led/version", QoS::at_most_once } }, 3);
	client.subscribe({ Subscribe_topic<String_view>{ "test/led/version", QoS::at_most_once } }, 4);

	while (stream) {
		std::error_code ec;

		// non-blocking approach
		const std::size_t available = stream.socket().lowest_layer().available() + stream.rdbuf()->in_avail();
		if (available > 0) {
			client.process_one(ec, available);
		}

		// blocking approach
		// client.process_one(ec);

		if (ec) {
			std::cerr << "Processing failed: " << ec.message() << "\n";
			break;
		}

		client.update_state();
		stream.flush();
		std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
	}
	std::cerr << "Stream broken\n";
} catch (const std::system_error& e) {
	// std::cerr << "exception caught (" << e.code() << "): " << e.what() << '\n';
	TERRAQTT_LOG(CRITICAL, "Exception from main ({}): {}", e.code().value(), e.what());
	return 1;
}
