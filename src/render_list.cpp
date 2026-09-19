#include "render_list.hpp"

#include "render_common.hpp"

namespace devgraph::render {

namespace {

void visit(std::ostream& out, const DeviceNode& node, const RenderOptions& opts) {
    if (node.parent != nullptr) { // skip the synthetic super-root
        const std::string& path = node.dt_only ? node.of_node_path : node.sysfs_path;
        out << path << "  " << build_label(node, opts) << "\n";
    }
    for (const auto& child : node.children) {
        if (child->visible) {
            visit(out, *child, opts);
        }
    }
}

} // namespace

void render_list(std::ostream& out, const DeviceNode& root, const RenderOptions& opts) {
    visit(out, root, opts);
}

} // namespace devgraph::render
