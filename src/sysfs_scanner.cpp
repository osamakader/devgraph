#include "sysfs_scanner.hpp"

#include "devnode_index.hpp"
#include "hwid_db.hpp"
#include "util.hpp"

#include <algorithm>
#include <map>
#include <system_error>

namespace fs = std::filesystem;

namespace devgraph {

namespace {

bool is_real_dir(const fs::directory_entry& entry) {
    std::error_code ec;
    // symlink_status() does not follow the link, so symlinks (subsystem,
    // driver, of_node, firmware_node, iommu_group, supplier:*, ...) are
    // correctly reported as "not a directory" here and never traversed.
    // This is what keeps the walk from looping or crossing into /sys/bus.
    auto st = fs::symlink_status(entry.path(), ec);
    return !ec && fs::is_directory(st);
}

std::vector<fs::path> sorted_subdirs(const fs::path& dir) {
    std::vector<fs::path> out;
    std::error_code ec;
    fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
    if (ec) {
        return out;
    }
    for (const auto& entry : it) {
        if (is_real_dir(entry)) {
            out.push_back(entry.path());
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

bool has_virtual_component(const fs::path& p) {
    for (const auto& part : p) {
        if (part == "virtual") {
            return true;
        }
    }
    return false;
}

void fill_device_fields(DeviceNode& node, const fs::path& dir) {
    node.sysfs_path = dir.string();
    node.name = dir.filename().string();
    node.subsystem = util::read_link_basename(dir / "subsystem").value_or("");
    node.driver = util::read_link_basename(dir / "driver").value_or("");
    node.is_virtual = has_virtual_component(dir);

    auto uevent = util::parse_uevent(dir / "uevent");
    for (const auto& [key, value] : uevent) {
        if (key == "MODALIAS") {
            node.modalias = value;
        } else if (key == "DEVTYPE") {
            node.devtype = value;
        } else if (key == "DRIVER" && node.driver.empty()) {
            node.driver = value;
        }
    }
    if (auto devnode = resolve_devnode(uevent)) {
        node.devnodes.push_back(*devnode);
    }

    if (auto of_target = util::resolve_link(dir / "of_node")) {
        node.of_node_path = of_target->string();
        if (auto compat = util::read_file(*of_target / "compatible")) {
            auto list = util::split_nul_list(*compat);
            if (!list.empty()) {
                node.of_compatible = list.front();
            }
        }
    }

    auto read_hex_attr = [&](const char* attr) -> std::optional<uint16_t> {
        auto text = util::read_attr(dir / attr);
        return text ? parse_hex_id(*text) : std::nullopt;
    };

    const auto& db = HwIdDb::instance();
    if (node.subsystem == "pci") {
        auto vendor = read_hex_attr("vendor");
        auto device = read_hex_attr("device");
        if (vendor && device) {
            node.product_name = db.pci_device_name(*vendor, *device).value_or("");
        }
    } else if (node.subsystem == "usb") {
        auto vendor = read_hex_attr("idVendor");
        auto product = read_hex_attr("idProduct");
        if (vendor && product) {
            node.product_name = db.usb_device_name(*vendor, *product).value_or("");
        }
    }
}

void walk(const fs::path& dir, DeviceNode* attach_point) {
    for (const auto& sub : sorted_subdirs(dir)) {
        std::error_code ec;
        bool has_uevent = fs::exists(sub / "uevent", ec) && !ec;
        if (has_uevent) {
            auto node = std::make_unique<DeviceNode>();
            fill_device_fields(*node, sub);
            node->parent = attach_point;
            DeviceNode* child_ptr = node.get();
            attach_point->children.push_back(std::move(node));
            walk(sub, child_ptr);
        } else {
            // Not itself a device (e.g. "power/", numbered irq dirs) --
            // flatten through it so real descendants still attach to the
            // nearest actual device ancestor.
            walk(sub, attach_point);
        }
    }
}

void collect_of_node_map(DeviceNode* node, std::map<std::string, DeviceNode*>& out) {
    if (!node->of_node_path.empty()) {
        out[node->of_node_path] = node;
    }
    for (auto& child : node->children) {
        collect_of_node_map(child.get(), out);
    }
}

// Merges device-tree nodes that have no corresponding bound sysfs device
// (disabled nodes, nodes with no driver, nodes the kernel doesn't
// instantiate a struct device for) into the tree as synthetic entries.
//
// `allow_synthetic` gates whether unmatched nodes may be attached here.
// It starts false for a scoped scan (sysfs_root narrower than the DT
// root's natural scope), since of_map only contains anchors found under
// sysfs_root -- an unmatched top-level DT node there could just as
// easily be scanner-unrelated hardware (e.g. "cpus", "memory") as it
// could be genuinely-unbound hardware under the scoped subtree. Once we
// descend through a real match (found != of_map.end()), we know we're
// inside the scoped subtree, so it flips true for all descendants.
void merge_dt_children(DtNode* dt, DeviceNode* attach_point, std::map<std::string, DeviceNode*>& of_map,
                        bool allow_synthetic) {
    for (auto& child : dt->children) {
        auto found = of_map.find(child->path);
        if (found != of_map.end()) {
            merge_dt_children(child.get(), found->second, of_map, true);
            continue;
        }
        if (!child->compatible.empty() && allow_synthetic) {
            auto node = std::make_unique<DeviceNode>();
            node->sysfs_path = "";
            node->name = child->name;
            node->subsystem = "devicetree";
            node->of_node_path = child->path;
            node->of_compatible = child->compatible;
            node->dt_only = true;
            node->dt_status = child->status;
            node->parent = attach_point;
            DeviceNode* child_ptr = node.get();
            attach_point->children.push_back(std::move(node));
            merge_dt_children(child.get(), child_ptr, of_map, true);
        } else {
            // Either a pure container node (e.g. "soc", "cpus") with no
            // device of its own, or a device node we can't place yet
            // because it fell outside the scoped scan. Keep recursing
            // without attaching, in case a deeper descendant is a real
            // match, but don't synthesize anything until one is found.
            merge_dt_children(child.get(), attach_point, of_map, allow_synthetic);
        }
    }
}

} // namespace

ScanResult scan(const ScanOptions& options) {
    ScanResult result;
    result.root = std::make_unique<DeviceNode>();
    result.root->name = options.sysfs_root.string();
    result.root->subsystem = "root";

    walk(options.sysfs_root, result.root.get());

    if (options.include_devicetree) {
        if (auto dt_root = scan_devicetree(options.devicetree_root)) {
            std::map<std::string, DeviceNode*> of_map;
            collect_of_node_map(result.root.get(), of_map);
            merge_dt_children(dt_root.get(), result.root.get(), of_map, !options.scoped_root);
        }
    }

    return result;
}

} // namespace devgraph
