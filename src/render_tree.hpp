#pragma once

#include "device_node.hpp"
#include "render_options.hpp"

#include <ostream>

namespace devgraph::render {

void render_tree(std::ostream& out, const DeviceNode& root, const RenderOptions& opts);

} // namespace devgraph::render
