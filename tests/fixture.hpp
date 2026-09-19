#pragma once

#include <filesystem>
#include <string>

// Helpers for building throwaway sysfs/devicetree-shaped directory trees
// on disk, so scanner/filter/render tests can exercise real
// std::filesystem traversal (symlinks, uevent files, DT compatible
// strings) without touching the real machine's /sys.

namespace devgraph::test {

// RAII temporary directory, removed recursively on destruction.
class TempDir {
public:
    TempDir();
    ~TempDir();
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

// Writes `content` to `path`, creating parent directories as needed.
void write_file(const std::filesystem::path& path, const std::string& content);

// Creates a symlink at `link` pointing at `target`, creating `link`'s
// parent directories as needed. `target` may be absolute or dangling.
void make_symlink(const std::filesystem::path& target, const std::filesystem::path& link);

} // namespace devgraph::test
