#include "framework.hpp"
#include "hwid_db.hpp"

using namespace devgraph;
using namespace devgraph::test;

DEVGRAPH_TEST(ParseHexIdAcceptsWithAndWithout0xPrefix) {
    auto a = parse_hex_id("0x8086");
    auto b = parse_hex_id("8086");
    CHECK(a.has_value());
    CHECK(b.has_value());
    if (a && b) {
        CHECK_EQ(*a, uint16_t{0x8086});
        CHECK_EQ(*b, uint16_t{0x8086});
    }
}

DEVGRAPH_TEST(ParseHexIdRejectsEmptyOrNonHex) {
    CHECK(!parse_hex_id("").has_value());
    CHECK(!parse_hex_id("not-hex").has_value());
}
