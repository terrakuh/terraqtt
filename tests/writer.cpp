#include <catch2/catch.hpp>
#include <cstring>
#include <terraqtt/protocol/writer.hpp>

using namespace terraqtt::protocol;

template<std::size_t Size>
inline void test(const char (&data)[Size], Variable_integer value)
{
	std::error_code ec;
	Byte buffer[4];

	INFO("value: " << static_cast<std::uint32_t>(value));
	REQUIRE(write_elements(buffer, ec, static_cast<Variable_integer>(value)) - buffer == Size - 1);
	REQUIRE(!ec);
	REQUIRE(!std::memcmp(buffer, data, Size - 1));
}

TEST_CASE("write variable integer")
{
	test("\x00", static_cast<Variable_integer>(0));
	test("\x7f", static_cast<Variable_integer>(0x7f));
	test("\x80\x01", static_cast<Variable_integer>(128));
	test("\xff\x7f", static_cast<Variable_integer>(16383));
	test("\x80\x80\x01", static_cast<Variable_integer>(16384));
	test("\xff\xff\x7f", static_cast<Variable_integer>(2097151));
	test("\x80\x80\x80\x01", static_cast<Variable_integer>(2097152));
	test("\xff\xff\xff\x7f", static_cast<Variable_integer>(268435455));
}
