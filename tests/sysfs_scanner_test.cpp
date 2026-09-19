#include "device_node.hpp"
#include "fixture.hpp"
#include "framework.hpp"
#include "sysfs_scanner.hpp"

#include <algorithm>

using namespace devgraph;
using namespace devgraph::test;

namespace {

DeviceNode* find_child(DeviceNode* node, const std::string& name) {
    for (auto& child : node->children) {
        if (child->name == name) {
            return child.get();
        }
    }
    return nullptr;
}

bool contains_name(DeviceNode* node, const std::string& name) {
    if (node->name == name) {
        return true;
    }
    for (auto& child : node->children) {
        if (contains_name(child.get(), name)) {
            return true;
        }
    }
    return false;
}

} // namespace

DEVGRAPH_TEST(WalkFlattensNonDeviceDirsAndSkipsSymlinkedSubdirs) {
    TempDir dir;
    auto root = dir.path() / "sysfs";

    // devA sits two levels down through "power/" and a numbered irq dir,
    // neither of which carries a uevent file -- both should be flattened
    // through so devA attaches directly to the synthetic root.
    write_file(root / "power" / "42" / "devA" / "uevent", "DRIVER=driverX\n");

    // A directory that looks like a device but is only reachable through a
    // symlink named "subsystem" must never be walked into (that's what
    // keeps the scan from looping back through /sys/bus).
    write_file(dir.path() / "elsewhere" / "devB" / "uevent", "DRIVER=driverB\n");
    make_symlink(dir.path() / "elsewhere", root / "subsystem");

    ScanOptions opts;
    opts.sysfs_root = root;
    opts.include_devicetree = false;
    ScanResult result = scan(opts);

    CHECK(find_child(result.root.get(), "devA") != nullptr);
    CHECK(!contains_name(result.root.get(), "devB"));

    auto* devA = find_child(result.root.get(), "devA");
    if (devA) {
        CHECK_EQ(devA->driver, std::string("driverX")); // fallback to uevent DRIVER= with no driver symlink
        CHECK(devA->parent == result.root.get());       // flattened, not nested under power/42/
    }
}

DEVGRAPH_TEST(FillDeviceFieldsResolvesOfNodeCompatible) {
    TempDir dir;
    auto sysfs_root = dir.path() / "sysfs";
    auto dt_root = dir.path() / "dt";

    write_file(dt_root / "node1" / "compatible", std::string("vendor,foo\0vendor,generic-foo\0", 30));
    write_file(sysfs_root / "devD" / "uevent", "");
    make_symlink(dt_root / "node1", sysfs_root / "devD" / "of_node");

    ScanOptions opts;
    opts.sysfs_root = sysfs_root;
    opts.include_devicetree = false;
    ScanResult result = scan(opts);

    auto* devD = find_child(result.root.get(), "devD");
    CHECK(devD != nullptr);
    if (devD) {
        CHECK_EQ(devD->of_compatible, std::string("vendor,foo"));
        CHECK_EQ(devD->of_node_path, std::filesystem::canonical(dt_root / "node1").string());
    }
}

namespace {

// Shared DT-merge fixture:
//   dt/cpus/cpu@0            compatible, no bound sysfs device
//   dt/soc/                  no compatible (pure container)
//   dt/soc/i2c@100           compatible, BOUND to sysfs device "soc-i2c"
//   dt/soc/i2c@100/eeprom@50 compatible, unbound child of a bound device
//   dt/soc/spi@200           compatible, unbound sibling of a bound device
struct DtMergeFixture {
    TempDir dir;
    std::filesystem::path sysfs_root = dir.path() / "sysfs";
    std::filesystem::path dt_root = std::filesystem::path();

    DtMergeFixture() {
        auto dt_root_raw = dir.path() / "dt";
        write_file(dt_root_raw / "cpus" / "cpu@0" / "compatible", std::string("arm,cortex-a53\0", 15));
        write_file(dt_root_raw / "soc" / "i2c@100" / "compatible", std::string("vendor,i2c\0", 11));
        write_file(dt_root_raw / "soc" / "i2c@100" / "eeprom@50" / "compatible", std::string("vendor,eeprom\0", 14));
        write_file(dt_root_raw / "soc" / "spi@200" / "compatible", std::string("vendor,spi\0", 11));
        dt_root = std::filesystem::canonical(dt_root_raw);

        write_file(sysfs_root / "soc-i2c" / "uevent", "");
        make_symlink(dt_root / "soc" / "i2c@100", sysfs_root / "soc-i2c" / "of_node");
    }

    ScanResult run(bool scoped_root) {
        ScanOptions opts;
        opts.sysfs_root = sysfs_root;
        opts.devicetree_root = dt_root;
        opts.include_devicetree = true;
        opts.scoped_root = scoped_root;
        return scan(opts);
    }
};

} // namespace

DEVGRAPH_TEST(UnscopedScanMergesAllUnboundDtNodesIncludingUnrelatedBranches) {
    DtMergeFixture fixture;
    ScanResult result = fixture.run(/*scoped_root=*/false);

    CHECK(contains_name(result.root.get(), "cpu@0"));    // unrelated top-level branch
    CHECK(contains_name(result.root.get(), "spi@200"));  // unmatched sibling of a bound device
    CHECK(contains_name(result.root.get(), "eeprom@50")); // unbound descendant of a bound device

    auto* soc_i2c = find_child(result.root.get(), "soc-i2c");
    CHECK(soc_i2c != nullptr);
    if (soc_i2c) {
        auto* eeprom = find_child(soc_i2c, "eeprom@50");
        CHECK(eeprom != nullptr);
        if (eeprom) {
            CHECK(eeprom->dt_only);
        }
    }
}

DEVGRAPH_TEST(ScopedRootPrunesUnrelatedDtBranchesButKeepsDescendantsOfScannedAnchors) {
    DtMergeFixture fixture;
    ScanResult result = fixture.run(/*scoped_root=*/true);

    // Neither is reachable from anything the scoped sysfs scan actually
    // found (cpu@0 is a wholly unrelated top-level branch; spi@200 is an
    // unmatched sibling of the one device that *was* found) -- both must
    // be pruned, not just left dangling off the scoped synthetic root.
    CHECK(!contains_name(result.root.get(), "cpu@0"));
    CHECK(!contains_name(result.root.get(), "spi@200"));

    // eeprom@50 IS a genuine descendant of "soc-i2c", which the scoped
    // scan did find -- it must still be merged in.
    auto* soc_i2c = find_child(result.root.get(), "soc-i2c");
    CHECK(soc_i2c != nullptr);
    if (soc_i2c) {
        CHECK(find_child(soc_i2c, "eeprom@50") != nullptr);
    }
}
