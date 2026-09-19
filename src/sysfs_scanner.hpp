#pragma once

#include "device_node.hpp"
#include "devicetree.hpp"

#include <filesystem>
#include <memory>

namespace devgraph {

struct ScanOptions {
    std::filesystem::path sysfs_root = "/sys/devices";
    std::filesystem::path devicetree_root = "/sys/firmware/devicetree/base";
    bool include_devicetree = true;

    // True when sysfs_root is a caller-narrowed subtree (e.g. --root) rather
    // than the full device tree's natural scope. When set, device-tree-only
    // nodes are only synthesized once merging has descended through a real
    // match, so unrelated DT branches outside the scanned subtree (siblings
    // like "cpus"/"memory" of a scoped "soc") don't get pulled in.
    bool scoped_root = false;
};

struct ScanResult {
    // Synthetic super-root. Its own fields are mostly unused; its
    // children are the top-level buses/devices found under sysfs_root.
    std::unique_ptr<DeviceNode> root;
};

// Walks sysfs_root, building one DeviceNode per directory that carries a
// "uevent" file, and merges in device-tree-only nodes (nodes present in
// the DT that the kernel hasn't bound/instantiated) when available.
ScanResult scan(const ScanOptions& options);

} // namespace devgraph
