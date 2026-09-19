#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace devgraph {

// A node from /sys/firmware/devicetree/base, kept separate from DeviceNode
// because most DT nodes are *not* device tree entries a driver has bound
// to yet -- they exist whether or not the kernel ever creates a struct
// device for them (disabled nodes, nodes with no matching driver, etc).
struct DtNode {
    std::string path;         // full path under /sys/firmware/devicetree/base
    std::string name;         // node name, e.g. "i2c@1c2ac00"
    std::string compatible;   // first string of the "compatible" property
    std::string status;       // "status" property, defaults to "okay" if absent
    std::vector<std::unique_ptr<DtNode>> children;
};

// Scans the live device tree exposed by the kernel, if any. Returns nullptr
// if the platform has no device tree (e.g. a PC/ACPI system).
std::unique_ptr<DtNode> scan_devicetree(const std::filesystem::path& dt_root = "/sys/firmware/devicetree/base");

// Flattens a DT tree into path -> node lookups, for matching against
// DeviceNode::of_node_path.
void index_devicetree(DtNode* root, std::map<std::string, DtNode*>& out);

} // namespace devgraph
