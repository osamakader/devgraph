#include "filter.hpp"

#include <algorithm>
#include <cctype>

namespace devgraph {

namespace {

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

bool matches_self(const DeviceNode& node, const FilterOptions& options) {
    if (!options.subsystems.empty()) {
        auto subsystem = to_lower(node.subsystem);
        bool found = std::any_of(options.subsystems.begin(), options.subsystems.end(),
                                  [&](const std::string& s) { return to_lower(s) == subsystem; });
        if (!found) {
            return false;
        }
    }
    if (options.driver && to_lower(*options.driver) != to_lower(node.driver)) {
        return false;
    }
    if (options.only_with_devnode && node.devnodes.empty()) {
        return false;
    }
    return true;
}

bool is_excluded(const DeviceNode& node, const FilterOptions& options) {
    if (options.hide_virtual && node.is_virtual) {
        return true;
    }
    if (options.exclude_subsystems.empty()) {
        return false;
    }
    auto subsystem = to_lower(node.subsystem);
    auto name = to_lower(node.name);
    return std::any_of(options.exclude_subsystems.begin(), options.exclude_subsystems.end(), [&](const std::string& s) {
        auto lowered = to_lower(s);
        // Some pseudo-bus containers (.../system/machinecheck,
        // .../system/clockevents, ...) have no "subsystem" symlink of
        // their own -- only their per-CPU children do. Matching the
        // node's own name too catches the now-empty container instead
        // of leaving a dead, info-less leaf behind.
        return lowered == subsystem || lowered == name;
    });
}

} // namespace

bool apply_filter(DeviceNode* node, const FilterOptions& options) {
    // Excluded devices are dropped outright, along with their whole
    // subtree: real hardware is never nested under a software/pseudo
    // device, so there is nothing worth preserving ancestor-context for
    // underneath.
    if (node->parent != nullptr && is_excluded(*node, options)) {
        node->visible = false;
        return false;
    }

    bool any_child_visible = false;
    for (auto& child : node->children) {
        if (apply_filter(child.get(), options)) {
            any_child_visible = true;
        }
    }

    bool self_visible = node->parent == nullptr || !options.has_positive_filter() ||
                         matches_self(*node, options) || any_child_visible;
    node->visible = self_visible;
    return self_visible;
}

} // namespace devgraph
