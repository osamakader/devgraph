#include "devnode_index.hpp"
#include "framework.hpp"

using namespace devgraph;
using namespace devgraph::test;

// resolve_devnode checks existence under the real /dev (it's not rooted
// under a scan's sysfs_root), so these tests rely only on device nodes
// virtually guaranteed to exist on any Linux system.

DEVGRAPH_TEST(ResolveDevnodeReturnsPathWhenDeviceExists) {
    std::vector<std::pair<std::string, std::string>> uevent = {{"MAJOR", "1"}, {"DEVNAME", "null"}};
    auto resolved = resolve_devnode(uevent);
    CHECK(resolved.has_value());
    if (resolved) {
        CHECK_EQ(*resolved, std::string("/dev/null"));
    }
}

DEVGRAPH_TEST(ResolveDevnodeReturnsNulloptWhenDeviceMissing) {
    std::vector<std::pair<std::string, std::string>> uevent = {{"DEVNAME", "devgraph_test_should_not_exist_xyz"}};
    CHECK(!resolve_devnode(uevent).has_value());
}

DEVGRAPH_TEST(ResolveDevnodeReturnsNulloptWhenNoDevnameKey) {
    std::vector<std::pair<std::string, std::string>> uevent = {{"MODALIAS", "of:Nfoo"}};
    CHECK(!resolve_devnode(uevent).has_value());
}
