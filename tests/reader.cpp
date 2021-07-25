#include <catch2/catch.hpp>
#include <sstream>
#include <terraqtt/protocol/reader.hpp>
#include <utility>

using namespace terraqtt::protocol;

template<std::size_t Size>
inline std::pair<std::stringstream, Read_context> create(const char (&data)[Size])
{
	Read_context ctx;
	std::stringstream ss;
	ss.write(data, Size - 1);
	ctx.available      = ss.tellp();
	ctx.remaining_size = ss.tellp();
	return { std::move(ss), ctx };
}

TEST_CASE("read variable integer")
{
	Read_context ctx;
	std::stringstream ss;
	std::error_code ec;
	Variable_integer value{};

	std::tie(ss, ctx) = create("\x00");
	REQUIRE(read_element(ss, ec, ctx, value));
	REQUIRE(static_cast<int>(value) == 0);
	REQUIRE(!ec);

	std::tie(ss, ctx) = create("\x7f");
	REQUIRE(read_element(ss, ec, ctx, value));
	REQUIRE(static_cast<int>(value) == 0x7f);
	REQUIRE(!ec);

	std::tie(ss, ctx) = create("\xff");
	REQUIRE(!read_element(ss, ec, ctx, value));
	REQUIRE(!ec);

	std::tie(ss, ctx) = create("\x80\x01");
	REQUIRE(read_element(ss, ec, ctx, value));
	REQUIRE(static_cast<int>(value) == 0x80);
	REQUIRE(!ec);

	std::tie(ss, ctx) = create("\x80\x80\x80\x01");
	REQUIRE(read_element(ss, ec, ctx, value));
	REQUIRE(static_cast<int>(value) == 2097152);
	REQUIRE(!ec);

	std::tie(ss, ctx) = create("\xff\xff\xff\x7f");
	REQUIRE(read_element(ss, ec, ctx, value));
	REQUIRE(static_cast<int>(value) == 268435455);
	REQUIRE(!ec);
}
