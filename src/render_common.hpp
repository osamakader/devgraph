#pragma once

#include "device_node.hpp"
#include "render_options.hpp"

#include <sstream>

namespace devgraph::render {

namespace color {
constexpr const char* kReset = "\033[0m";
constexpr const char* kDim = "\033[2m";
constexpr const char* kSubsystem = "\033[36m";  // cyan
constexpr const char* kDriver = "\033[32m";     // green
constexpr const char* kDevnode = "\033[33m";    // yellow
constexpr const char* kUnbound = "\033[2;31m";  // dim red
constexpr const char* kCompatible = "\033[35m"; // magenta
} // namespace color

inline std::string colorize(const RenderOptions& opts, const char* code, const std::string& text) {
    if (!opts.use_color || text.empty()) {
        return text;
    }
    return std::string(code) + text + color::kReset;
}

// Builds the human-readable one-line label for a device: name plus any
// requested annotations (subsystem, driver, devnode, DT compatible).
inline std::string build_label(const DeviceNode& node, const RenderOptions& opts) {
    std::ostringstream out;

    if (node.dt_only) {
        out << colorize(opts, color::kUnbound, node.name);
    } else {
        out << node.name;
    }

    if (opts.show_subsystem && !node.subsystem.empty() && node.subsystem != "root") {
        out << " [" << colorize(opts, color::kSubsystem, node.subsystem) << "]";
    }
    if (opts.show_driver && !node.driver.empty()) {
        out << " driver=" << colorize(opts, color::kDriver, node.driver);
    }
    if (opts.show_compatible && !node.of_compatible.empty()) {
        out << " compatible=" << colorize(opts, color::kCompatible, node.of_compatible);
    }
    if (opts.show_devnode) {
        for (const auto& dev : node.devnodes) {
            out << " -> " << colorize(opts, color::kDevnode, dev);
        }
    }
    if (node.dt_only) {
        out << colorize(opts, color::kDim, node.dt_status == "okay" ? " (unbound)" : " (unbound, " + node.dt_status + ")");
    }
    return out.str();
}

} // namespace devgraph::render
