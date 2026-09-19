#pragma once

#include <memory>
#include <string>
#include <vector>

namespace devgraph {

// A single node in the hardware topology graph. Corresponds to one
// directory under /sys/devices that carries a "uevent" file (i.e. an
// actual struct device, not just an attribute sub-directory).
struct DeviceNode {
    std::string sysfs_path;    // full path under /sys/devices
    std::string name;          // basename of sysfs_path
    std::string subsystem;     // e.g. i2c, spi, usb, pci, platform, mmc, virtio
    std::string driver;        // bound driver name, empty if unbound
    std::string modalias;      // MODALIAS= from uevent, if present
    std::string devtype;       // DEVTYPE= from uevent, if present
    std::string product_name;  // human-readable name from pci.ids/usb.ids, if resolved

    std::string of_node_path;    // resolved /sys/firmware/devicetree/base/... path
    std::string of_compatible;   // first "compatible" string from the DT node

    std::vector<std::string> devnodes; // associated /dev/* paths

    DeviceNode* parent = nullptr;
    std::vector<std::unique_ptr<DeviceNode>> children;

    bool dt_only = false;   // true if this node has no bound sysfs device
    std::string dt_status;  // device-tree "status" property, for dt_only nodes

    bool is_virtual = false; // true if this lives under .../devices/virtual/...

    bool visible = true; // used by filtering to prune subtrees for rendering
};

} // namespace devgraph
