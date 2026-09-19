#include "render_dot.hpp"

#include <sstream>
#include <unordered_map>

namespace devgraph::render {

namespace {

// Escapes a label for DOT's quoted-string syntax. Real newlines (used to
// stack lines within a label) become the literal two-character "\n" that
// DOT expects; backslashes and quotes are escaped so they aren't
// mistaken for that same escape syntax.
std::string escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\n') {
            out += "\\n";
        } else if (c == '"' || c == '\\') {
            out.push_back('\\');
            out.push_back(c);
        } else {
            out.push_back(c);
        }
    }
    return out;
}

// Stable, deterministic per-subsystem colors without pulling in a palette
// dependency: hash the subsystem name into a hue and print HSV directly,
// which Graphviz accepts natively.
std::string color_for_subsystem(const std::string& subsystem) {
    if (subsystem.empty()) {
        return "0.0,0.0,0.85";
    }
    size_t h = std::hash<std::string>{}(subsystem);
    double hue = static_cast<double>(h % 360) / 360.0;
    return std::to_string(hue) + ",0.35,0.95";
}

std::string label_for(const DeviceNode& node) {
    std::ostringstream label;
    label << node.name;
    if (!node.subsystem.empty() && node.subsystem != "root") {
        label << "\n[" << node.subsystem << "]";
    }
    if (!node.product_name.empty()) {
        label << "\n" << node.product_name;
    }
    if (!node.driver.empty()) {
        label << "\ndriver: " << node.driver;
    }
    if (!node.of_compatible.empty()) {
        label << "\ncompatible: " << node.of_compatible;
    }
    for (const auto& dev : node.devnodes) {
        label << "\n" << dev;
    }
    if (node.dt_only) {
        label << "\n(unbound)";
    }
    return label.str();
}

void emit(std::ostream& out, const DeviceNode& node, std::unordered_map<const DeviceNode*, int>& ids, int& next_id) {
    int id = next_id++;
    ids[&node] = id;

    if (node.parent != nullptr) { // synthetic super-root has no node of its own
        out << "  n" << id << " [label=\"" << escape(label_for(node)) << "\""
            << " style=\"filled" << (node.dt_only ? ",dashed" : "") << "\""
            << " fillcolor=\"" << color_for_subsystem(node.subsystem) << "\"];\n";
    }

    for (const auto& child : node.children) {
        if (!child->visible) {
            continue;
        }
        emit(out, *child, ids, next_id);
        int child_id = ids[child.get()];
        if (node.parent != nullptr) {
            out << "  n" << id << " -> n" << child_id << ";\n";
        } else {
            out << "  n" << child_id << ";\n"; // top-level bus, no parent edge to draw
        }
    }
}

} // namespace

void render_dot(std::ostream& out, const DeviceNode& root, const RenderOptions&) {
    out << "digraph devgraph {\n";
    out << "  rankdir=LR;\n";
    out << "  node [shape=box, fontname=\"monospace\", fontsize=10];\n";

    std::unordered_map<const DeviceNode*, int> ids;
    int next_id = 0;
    emit(out, root, ids, next_id);

    out << "}\n";
}

} // namespace devgraph::render
