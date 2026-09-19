#include "device_node.hpp"
#include "filter.hpp"
#include "framework.hpp"

using namespace devgraph;
using namespace devgraph::test;

namespace {

// Builds:
//   root
//     busA [i2c]
//       devA [i2c] driver=drv1
//       devB [i2c] driver=drv2
//     busB [spi]
//       devC [spi]
DeviceNode* add_child(DeviceNode* parent, std::string name, std::string subsystem, std::string driver = "") {
    auto node = std::make_unique<DeviceNode>();
    node->name = std::move(name);
    node->subsystem = std::move(subsystem);
    node->driver = std::move(driver);
    node->parent = parent;
    DeviceNode* ptr = node.get();
    parent->children.push_back(std::move(node));
    return ptr;
}

struct Tree {
    std::unique_ptr<DeviceNode> root = std::make_unique<DeviceNode>();
    DeviceNode* busA;
    DeviceNode* devA;
    DeviceNode* devB;
    DeviceNode* busB;
    DeviceNode* devC;

    Tree() {
        busA = add_child(root.get(), "busA", "i2c");
        devA = add_child(busA, "devA", "i2c", "drv1");
        devB = add_child(busA, "devB", "i2c", "drv2");
        busB = add_child(root.get(), "busB", "spi");
        devC = add_child(busB, "devC", "spi");
    }
};

} // namespace

DEVGRAPH_TEST(NoFilterKeepsEverythingVisible) {
    Tree t;
    apply_filter(t.root.get(), FilterOptions{});
    CHECK(t.root->visible);
    CHECK(t.busA->visible);
    CHECK(t.devA->visible);
    CHECK(t.devB->visible);
    CHECK(t.busB->visible);
    CHECK(t.devC->visible);
}

DEVGRAPH_TEST(SubsystemFilterKeepsAncestorsOfMatches) {
    Tree t;
    FilterOptions opts;
    opts.subsystems = {"i2c"};
    apply_filter(t.root.get(), opts);

    CHECK(t.root->visible);
    CHECK(t.busA->visible); // ancestor of matches
    CHECK(t.devA->visible);
    CHECK(t.devB->visible);
    CHECK(!t.busB->visible);
    CHECK(!t.devC->visible);
}

DEVGRAPH_TEST(DriverFilterIsCaseInsensitiveAndKeepsAncestors) {
    Tree t;
    FilterOptions opts;
    opts.driver = "DRV1";
    apply_filter(t.root.get(), opts);

    CHECK(t.root->visible);
    CHECK(t.busA->visible);
    CHECK(t.devA->visible);
    CHECK(!t.devB->visible);
    CHECK(!t.busB->visible);
    CHECK(!t.devC->visible);
}

DEVGRAPH_TEST(WithDevnodeFilterRequiresAtLeastOneDevnode) {
    Tree t;
    t.devB->devnodes.push_back("/dev/devB0");
    FilterOptions opts;
    opts.only_with_devnode = true;
    apply_filter(t.root.get(), opts);

    CHECK(!t.devA->visible);
    CHECK(t.devB->visible);
    CHECK(t.busA->visible); // ancestor of a match
}

DEVGRAPH_TEST(ExcludeSubsystemDropsWholeSubtreeEvenWithoutPositiveFilter) {
    Tree t;
    FilterOptions opts;
    opts.exclude_subsystems = {"spi"};
    apply_filter(t.root.get(), opts);

    CHECK(t.root->visible);
    CHECK(t.busA->visible);
    CHECK(t.devA->visible);
    CHECK(t.devB->visible);
    // busB is excluded outright; apply_filter short-circuits before
    // recursing into its children (render only visits a node's children
    // once the node itself is visible), so devC's own flag is untouched
    // and irrelevant -- busB being hidden is what makes it unreachable.
    CHECK(!t.busB->visible);
}

DEVGRAPH_TEST(ExcludeSubsystemAlsoMatchesNodeNameForNamelessContainers) {
    Tree t;
    // A pseudo-bus container with no subsystem symlink of its own, only
    // identifiable by name (e.g. .../system/clockevents).
    t.busB->subsystem.clear();
    FilterOptions opts;
    opts.exclude_subsystems = {"busB"};
    apply_filter(t.root.get(), opts);

    CHECK(!t.busB->visible);
}

DEVGRAPH_TEST(VirtualNodesHiddenByDefaultAndShownWhenRequested) {
    Tree t;
    t.devC->is_virtual = true;

    apply_filter(t.root.get(), FilterOptions{});
    CHECK(!t.devC->visible);

    FilterOptions opts;
    opts.hide_virtual = false;
    apply_filter(t.root.get(), opts);
    CHECK(t.devC->visible);
}
