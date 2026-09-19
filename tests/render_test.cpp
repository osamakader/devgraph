#include "device_node.hpp"
#include "framework.hpp"
#include "render_common.hpp"
#include "render_dot.hpp"
#include "render_list.hpp"
#include "render_options.hpp"
#include "render_tree.hpp"

#include <sstream>

using namespace devgraph;
using namespace devgraph::test;

namespace {

std::unique_ptr<DeviceNode> make_annotated_device(std::string name) {
    auto node = std::make_unique<DeviceNode>();
    node->name = std::move(name);
    node->subsystem = "i2c";
    node->driver = "drv1";
    node->of_compatible = "vendor,foo";
    node->product_name = "Vendor Widget";
    node->devnodes = {"/dev/foo0"};
    return node;
}

} // namespace

DEVGRAPH_TEST(BuildLabelIncludesAllAnnotationsByDefault) {
    auto node = make_annotated_device("devA");
    RenderOptions opts;
    opts.use_color = false;
    auto label = render::build_label(*node, opts);
    CHECK(label.find("driver=drv1") != std::string::npos);
    CHECK(label.find("compatible=vendor,foo") != std::string::npos);
    CHECK(label.find("/dev/foo0") != std::string::npos);
    CHECK(label.find("Vendor Widget") != std::string::npos);
    CHECK(label.find("[i2c]") != std::string::npos);
}

DEVGRAPH_TEST(BuildLabelHonorsToggles) {
    auto node = make_annotated_device("devA");
    RenderOptions opts;
    opts.use_color = false;
    opts.show_driver = false;
    opts.show_devnode = false;
    opts.show_compatible = false;
    opts.show_product = false;
    auto label = render::build_label(*node, opts);
    CHECK(label.find("driver=") == std::string::npos);
    CHECK(label.find("compatible=") == std::string::npos);
    CHECK(label.find("/dev/foo0") == std::string::npos);
    CHECK(label.find("Vendor Widget") == std::string::npos);
}

DEVGRAPH_TEST(BuildLabelMarksUnboundDtOnlyNodes) {
    auto node = std::make_unique<DeviceNode>();
    node->name = "eeprom@50";
    node->dt_only = true;
    node->dt_status = "disabled";
    RenderOptions opts;
    opts.use_color = false;
    auto label = render::build_label(*node, opts);
    CHECK(label.find("(unbound, disabled)") != std::string::npos);
}

DEVGRAPH_TEST(RenderTreeSkipsHiddenChildren) {
    auto root = std::make_unique<DeviceNode>();
    root->name = "root";
    auto visible_child = std::make_unique<DeviceNode>();
    visible_child->name = "visibleDev";
    visible_child->parent = root.get();
    auto hidden_child = std::make_unique<DeviceNode>();
    hidden_child->name = "hiddenDev";
    hidden_child->parent = root.get();
    hidden_child->visible = false;
    root->children.push_back(std::move(visible_child));
    root->children.push_back(std::move(hidden_child));

    std::ostringstream out;
    render::render_tree(out, *root, RenderOptions{});
    CHECK(out.str().find("visibleDev") != std::string::npos);
    CHECK(out.str().find("hiddenDev") == std::string::npos);
}

DEVGRAPH_TEST(RenderListSkipsSyntheticRootAndUsesOfNodePathForDtOnlyNodes) {
    auto root = std::make_unique<DeviceNode>();
    root->name = "root";
    root->sysfs_path = "/sys/devices";

    auto dt_only = std::make_unique<DeviceNode>();
    dt_only->name = "eeprom@50";
    dt_only->dt_only = true;
    dt_only->of_node_path = "/sys/firmware/devicetree/base/soc/eeprom@50";
    dt_only->parent = root.get();
    root->children.push_back(std::move(dt_only));

    std::ostringstream out;
    render::render_list(out, *root, RenderOptions{});
    auto text = out.str();
    CHECK(text.find("/sys/devices") == std::string::npos); // synthetic root not listed
    CHECK(text.find("/sys/firmware/devicetree/base/soc/eeprom@50") != std::string::npos);
}

DEVGRAPH_TEST(RenderDotEscapesQuotesBackslashesAndNewlines) {
    auto root = std::make_unique<DeviceNode>();
    root->name = "root";
    auto child = std::make_unique<DeviceNode>();
    child->name = "dev\"quote\\slash";
    child->parent = root.get();
    root->children.push_back(std::move(child));

    std::ostringstream out;
    render::render_dot(out, *root, RenderOptions{});
    auto text = out.str();
    CHECK(text.find("dev\\\"quote\\\\slash") != std::string::npos);
    CHECK(text.find("digraph devgraph") != std::string::npos);
}

DEVGRAPH_TEST(RenderDotHonorsAnnotationToggles) {
    auto root = std::make_unique<DeviceNode>();
    root->name = "root";
    auto child = make_annotated_device("devA");
    child->parent = root.get();
    root->children.push_back(std::move(child));

    RenderOptions opts;
    opts.show_driver = false;
    opts.show_devnode = false;
    opts.show_compatible = false;
    opts.show_product = false;

    std::ostringstream out;
    render::render_dot(out, *root, opts);
    auto text = out.str();
    CHECK(text.find("driver:") == std::string::npos);
    CHECK(text.find("compatible:") == std::string::npos);
    CHECK(text.find("/dev/foo0") == std::string::npos);
    CHECK(text.find("Vendor Widget") == std::string::npos);

    std::ostringstream out_default;
    render::render_dot(out_default, *root, RenderOptions{});
    auto default_text = out_default.str();
    CHECK(default_text.find("driver: drv1") != std::string::npos);
    CHECK(default_text.find("compatible: vendor,foo") != std::string::npos);
    CHECK(default_text.find("/dev/foo0") != std::string::npos);
}
