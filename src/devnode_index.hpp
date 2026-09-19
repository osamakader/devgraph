#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace devgraph {

// Given a device's parsed uevent key/value pairs, returns the absolute
// /dev path the kernel advertised for it (via DEVNAME=), if that path
// actually exists on the running system.
std::optional<std::string> resolve_devnode(const std::vector<std::pair<std::string, std::string>>& uevent);

} // namespace devgraph
