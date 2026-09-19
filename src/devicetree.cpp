#include "devicetree.hpp"

#include "util.hpp"

#include <algorithm>
#include <array>
#include <system_error>

namespace devgraph {

namespace {

// DT sub-trees that are metadata/bookkeeping rather than hardware, and
// whose children (if any) are never real devices either.
constexpr std::array<const char*, 5> kSkipSubtrees = {
    "aliases", "chosen", "__symbols__", "__fixups__", "__local_fixups__",
};

bool should_skip(const std::string& name) {
    return std::find_if(kSkipSubtrees.begin(), kSkipSubtrees.end(), [&](const char* s) {
               return name == s;
           }) != kSkipSubtrees.end();
}

std::unique_ptr<DtNode> scan_node(const std::filesystem::path& dir) {
    auto node = std::make_unique<DtNode>();
    node->path = dir.string();
    node->name = dir.filename().string();
    if (node->name.empty()) {
        node->name = "/";
    }

    if (auto compat = util::read_file(dir / "compatible")) {
        auto list = util::split_nul_list(*compat);
        if (!list.empty()) {
            node->compatible = list.front();
        }
    }
    node->status = util::read_attr(dir / "status").value_or("okay");

    std::error_code ec;
    std::vector<std::filesystem::path> subdirs;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) {
            break;
        }
        std::error_code type_ec;
        if (entry.is_directory(type_ec) && !type_ec) {
            subdirs.push_back(entry.path());
        }
    }
    std::sort(subdirs.begin(), subdirs.end());

    for (const auto& sub : subdirs) {
        if (should_skip(sub.filename().string())) {
            continue;
        }
        node->children.push_back(scan_node(sub));
    }
    return node;
}

} // namespace

std::unique_ptr<DtNode> scan_devicetree(const std::filesystem::path& dt_root) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dt_root, ec) || ec) {
        return nullptr;
    }
    return scan_node(dt_root);
}

void index_devicetree(DtNode* root, std::map<std::string, DtNode*>& out) {
    if (!root) {
        return;
    }
    out[root->path] = root;
    for (auto& child : root->children) {
        index_devicetree(child.get(), out);
    }
}

} // namespace devgraph
