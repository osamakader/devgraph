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

    // Devices under .../devices/virtual/... (loop, ram, dm, tty*, bdi, ...)
    // are software constructs, not real hardware topology; hidden unless
    // the caller explicitly asks to see them.
    bool hide_virtual = true;

    // Subsystems to drop outright (whole subtree), regardless of the
    // virtual check above -- e.g. "event_source" (perf PMU pseudo-devices
    // like uncore_*/tracepoint/uprobe), which live outside /virtual but
    // are still instrumentation, not hardware.
    std::vector<std::string> exclude_subsystems;

    bool has_positive_filter() const {
        return !subsystems.empty() || driver.has_value() || only_with_devnode;
    }
};

// Marks DeviceNode::visible on every node in the tree: a node is visible
// if it matches the filter itself, or is an ancestor of a node that does
// (so the surrounding topology stays legible), or is the synthetic root.
// Returns true if `node` (or something under it) is visible.
bool apply_filter(DeviceNode* node, const FilterOptions& options);

} // namespace devgraph
