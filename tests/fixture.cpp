#include "fixture.hpp"

#include <fstream>
#include <random>
#include <sstream>
#include <system_error>

namespace devgraph::test {

namespace {

std::filesystem::path make_unique_dir() {
    static std::mt19937_64 rng{std::random_device{}()};
    auto base = std::filesystem::temp_directory_path();
    for (;;) {
        std::ostringstream name;
        name << "devgraph_test_" << rng();
        auto candidate = base / name.str();
        std::error_code ec;
        if (std::filesystem::create_directory(candidate, ec)) {
            return std::filesystem::canonical(candidate);
        }
    }
}

} // namespace

TempDir::TempDir() : path_(make_unique_dir()) {}

TempDir::~TempDir() {
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
}

void write_file(const std::filesystem::path& path, const std::string& content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << content;
}

void make_symlink(const std::filesystem::path& target, const std::filesystem::path& link) {
    std::filesystem::create_directories(link.parent_path());
    std::error_code ec;
    if (std::filesystem::is_directory(target, ec)) {
        std::filesystem::create_directory_symlink(target, link);
    } else {
        std::filesystem::create_symlink(target, link);
    }
}

} // namespace devgraph::test
