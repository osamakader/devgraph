#include "util.hpp"

#include <fstream>
#include <sstream>
#include <system_error>
#include <unistd.h>

namespace devgraph::util {

std::optional<std::string> read_file(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        return std::nullopt;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    if (in.bad()) {
        return std::nullopt;
    }
    return ss.str();
}

std::optional<std::string> read_attr(const std::filesystem::path& path) {
    auto content = read_file(path);
    if (!content) {
        return std::nullopt;
    }
    while (!content->empty() && (content->back() == '\n' || content->back() == '\r')) {
        content->pop_back();
    }
    return content;
}

std::vector<std::string> split_nul_list(const std::string& raw) {
    std::vector<std::string> out;
    std::string current;
    for (char c : raw) {
        if (c == '\0') {
            if (!current.empty()) {
                out.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        out.push_back(current);
    }
    return out;
}

std::optional<std::filesystem::path> resolve_link(const std::filesystem::path& link) {
    std::error_code ec;
    if (!std::filesystem::is_symlink(link, ec) || ec) {
        return std::nullopt;
    }
    auto target = std::filesystem::canonical(link, ec);
    if (ec) {
        return std::nullopt;
    }
    return target;
}

std::optional<std::string> read_link_basename(const std::filesystem::path& link) {
    auto target = resolve_link(link);
    if (!target) {
        return std::nullopt;
    }
    return target->filename().string();
}

std::vector<std::pair<std::string, std::string>> parse_uevent(const std::filesystem::path& uevent_path) {
    std::vector<std::pair<std::string, std::string>> out;
    auto content = read_file(uevent_path);
    if (!content) {
        return out;
    }
    std::istringstream stream(*content);
    std::string line;
    while (std::getline(stream, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        out.emplace_back(line.substr(0, eq), line.substr(eq + 1));
    }
    return out;
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::string current;
    for (char c : s) {
        if (c == delim) {
            out.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    out.push_back(current);
    return out;
}

bool stdout_is_tty() {
    return isatty(fileno(stdout)) != 0;
}

} // namespace devgraph::util
