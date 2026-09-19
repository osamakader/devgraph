#include "framework.hpp"
#include "fixture.hpp"
#include "util.hpp"

using namespace devgraph;
using namespace devgraph::test;

DEVGRAPH_TEST(SplitNulListDropsTrailingEmptyAndSplitsOnNul) {
    std::string raw("vendor,foo\0vendor,generic-foo\0", 30);
    auto parts = util::split_nul_list(raw);
    CHECK_EQ(parts.size(), size_t{2});
    if (parts.size() == 2) {
        CHECK_EQ(parts[0], std::string("vendor,foo"));
        CHECK_EQ(parts[1], std::string("vendor,generic-foo"));
    }
}

DEVGRAPH_TEST(SplitNulListEmptyInputYieldsNoParts) {
    auto parts = util::split_nul_list("");
    CHECK(parts.empty());
}

DEVGRAPH_TEST(SplitGenericHandlesEmptyFieldsAndTrailingDelimiter) {
    auto parts = util::split("a,,b,", ',');
    CHECK_EQ(parts.size(), size_t{4});
    if (parts.size() == 4) {
        CHECK_EQ(parts[0], std::string("a"));
        CHECK_EQ(parts[1], std::string(""));
        CHECK_EQ(parts[2], std::string("b"));
        CHECK_EQ(parts[3], std::string(""));
    }
}

DEVGRAPH_TEST(ReadAttrTrimsTrailingNewline) {
    TempDir dir;
    auto path = dir.path() / "attr";
    write_file(path, "i2c\n");
    auto value = util::read_attr(path);
    CHECK(value.has_value());
    if (value) {
        CHECK_EQ(*value, std::string("i2c"));
    }
}

DEVGRAPH_TEST(ReadFileReturnsNulloptForMissingFile) {
    TempDir dir;
    auto value = util::read_file(dir.path() / "does_not_exist");
    CHECK(!value.has_value());
}

DEVGRAPH_TEST(ParseUeventParsesKeyValueLinesAndIgnoresLinesWithoutEquals) {
    TempDir dir;
    auto path = dir.path() / "uevent";
    write_file(path, "MODALIAS=of:Nfoo\nnotakeyvalueline\nDEVTYPE=gpio\n");
    auto pairs = util::parse_uevent(path);
    CHECK_EQ(pairs.size(), size_t{2});
    if (pairs.size() == 2) {
        CHECK_EQ(pairs[0].first, std::string("MODALIAS"));
        CHECK_EQ(pairs[0].second, std::string("of:Nfoo"));
        CHECK_EQ(pairs[1].first, std::string("DEVTYPE"));
        CHECK_EQ(pairs[1].second, std::string("gpio"));
    }
}

DEVGRAPH_TEST(ResolveLinkFollowsSymlinkToCanonicalTarget) {
    TempDir dir;
    auto target = dir.path() / "real_target";
    std::filesystem::create_directory(target);
    auto link = dir.path() / "link_to_target";
    make_symlink(target, link);

    auto resolved = util::resolve_link(link);
    CHECK(resolved.has_value());
    if (resolved) {
        CHECK_EQ(resolved->string(), std::filesystem::canonical(target).string());
    }
}

DEVGRAPH_TEST(ResolveLinkReturnsNulloptForNonSymlink) {
    TempDir dir;
    auto plain_dir = dir.path() / "plain";
    std::filesystem::create_directory(plain_dir);
    CHECK(!util::resolve_link(plain_dir).has_value());
}

DEVGRAPH_TEST(ReadLinkBasenameReturnsFinalPathComponent) {
    TempDir dir;
    auto target = dir.path() / "bus" / "i2c";
    std::filesystem::create_directories(target);
    auto link = dir.path() / "device" / "subsystem";
    make_symlink(target, link);

    auto basename = util::read_link_basename(link);
    CHECK(basename.has_value());
    if (basename) {
        CHECK_EQ(*basename, std::string("i2c"));
    }
}
