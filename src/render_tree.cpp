#include "render_tree.hpp"

#include "render_common.hpp"

#include <vector>

namespace devgraph::render {

namespace {

std::vector<const DeviceNode*> visible_children(const DeviceNode& node) {
    std::vector<const DeviceNode*> out;
    for (const auto& child : node.children) {
        if (child->visible) {
            out.push_back(child.get());
        }
    }
    return out;
}

void print_node(std::ostream& out, const DeviceNode& node, const RenderOptions& opts, const std::string& prefix,
                 bool is_last) {
    out << prefix << (is_last ? "└── " : "├── ") << build_label(node, opts) << "\n";

    auto children = visible_children(node);
    std::string child_prefix = prefix + (is_last ? "    " : "│   ");
    for (size_t i = 0; i < children.size(); ++i) {
        print_node(out, *children[i], opts, child_prefix, i + 1 == children.size());
    }
}

} // namespace

void render_tree(std::ostream& out, const DeviceNode& root, const RenderOptions& opts) {
    out << root.name << "\n";
    auto children = visible_children(root);
    for (size_t i = 0; i < children.size(); ++i) {
        print_node(out, *children[i], opts, "", i + 1 == children.size());
    }
}

} // namespace devgraph::render
