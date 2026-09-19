#pragma once

#include "device_node.hpp"

#include <optional>
#include <string>
#include <vector>

namespace devgraph {

struct FilterOptions {
    std::vector<std::string> subsystems; // empty = no filter
    std::optional<std::string> driver;
    bool only_with_devnode = false;

    bool empty() const {
        return subsystems.empty() && !driver.has_value() && !only_with_devnode;
    }
};

// Marks DeviceNode::visible on every node in the tree: a node is visible
// if it matches the filter itself, or is an ancestor of a node that does
// (so the surrounding topology stays legible), or is the synthetic root.
// Returns true if `node` (or something under it) is visible.
bool apply_filter(DeviceNode* node, const FilterOptions& options);

} // namespace devgraph
