#pragma once

#include "device_node.hpp"
#include "render_options.hpp"

#include <ostream>

namespace devgraph::render {

// Emits a Graphviz DOT graph, suitable for `devgraph --format dot | dot -Tsvg -o topology.svg`.
void render_dot(std::ostream& out, const DeviceNode& root, const RenderOptions& opts);

} // namespace devgraph::render
