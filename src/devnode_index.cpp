#include "devnode_index.hpp"

#include <filesystem>
#include <system_error>

namespace devgraph {

std::optional<std::string> resolve_devnode(const std::vector<std::pair<std::string, std::string>>& uevent) {
    for (const auto& [key, value] : uevent) {
        if (key != "DEVNAME") {
            continue;
        }
        std::filesystem::path candidate = std::filesystem::path("/dev") / value;
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && !ec) {
            return candidate.string();
        }
        return std::nullopt;
    }
    return std::nullopt;
}

} // namespace devgraph
