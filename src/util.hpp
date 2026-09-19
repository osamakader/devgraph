#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace devgraph::util {

// Reads a whole regular file into a string. Returns std::nullopt if the
// file cannot be opened (missing, permission denied, etc.) rather than
// throwing, since /sys is full of files that are conditionally present.
std::optional<std::string> read_file(const std::filesystem::path& path);

// Reads a sysfs attribute file and trims a single trailing newline.
std::optional<std::string> read_attr(const std::filesystem::path& path);

// Parses a NUL-separated string list, as used by device-tree "compatible"
// and similar properties. Trailing empty strings are dropped.
std::vector<std::string> split_nul_list(const std::string& raw);

// Resolves a symlink and returns just the final path component
// (e.g. "subsystem" -> ".../bus/i2c" gives "i2c").
std::optional<std::string> read_link_basename(const std::filesystem::path& link);

// Resolves a symlink to its canonical absolute target path.
// Returns std::nullopt if the link is missing or dangling.
std::optional<std::filesystem::path> resolve_link(const std::filesystem::path& link);

// Parses a sysfs "uevent" file into key/value pairs (KEY=VALUE per line).
std::vector<std::pair<std::string, std::string>> parse_uevent(const std::filesystem::path& uevent_path);

std::vector<std::string> split(const std::string& s, char delim);

bool stdout_is_tty();

} // namespace devgraph::util
