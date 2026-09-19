#pragma once

#include "device_node.hpp"
#include "render_options.hpp"

#include <ostream>

namespace devgraph::render {

// Flat listing, one device per line, as "/full/sysfs/path  [annotations]".
void render_list(std::ostream& out, const DeviceNode& root, const RenderOptions& opts);

} // namespace devgraph::render
