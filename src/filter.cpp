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

} // namespace

bool apply_filter(DeviceNode* node, const FilterOptions& options) {
    if (options.empty()) {
        node->visible = true;
        for (auto& child : node->children) {
            apply_filter(child.get(), options);
        }
        return true;
    }

    bool any_child_visible = false;
    for (auto& child : node->children) {
        if (apply_filter(child.get(), options)) {
            any_child_visible = true;
        }
    }

    bool self_visible = node->parent == nullptr || matches_self(*node, options) || any_child_visible;
    node->visible = self_visible;
    return self_visible;
}

} // namespace devgraph
