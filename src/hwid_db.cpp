#include "hwid_db.hpp"

#include "util.hpp"

#include <array>
#include <cctype>
#include <charconv>
#include <filesystem>
#include <sstream>
#include <unordered_map>

namespace devgraph {

namespace {

constexpr std::array<const char*, 3> kPciIdsPaths = {
    "/usr/share/hwdata/pci.ids",
    "/usr/share/misc/pci.ids",
    "/var/lib/pciutils/pci.ids",
};

constexpr std::array<const char*, 3> kUsbIdsPaths = {
    "/usr/share/hwdata/usb.ids",
    "/usr/share/misc/usb.ids",
    "/var/lib/usbutils/usb.ids",
};

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

uint32_t composite_key(uint16_t vendor, uint16_t device) {
    return (static_cast<uint32_t>(vendor) << 16) | device;
}

struct IdTables {
    std::unordered_map<uint16_t, std::string> vendors;
    std::unordered_map<uint32_t, std::string> devices;
};

// pci.ids and usb.ids share the same tab-indented format:
//   XXXX  Vendor Name
//   \tYYYY  Device Name
//   \t\tSSSS TTTT  Subsystem Name   (skipped; we don't need it)
// and end with a "C  class ..." section we don't care about at all.
IdTables parse_ids_file(const std::filesystem::path& path) {
    IdTables tables;
    auto content = util::read_file(path);
    if (!content) {
        return tables;
    }

    std::istringstream stream(*content);
    std::string line;
    uint16_t current_vendor = 0;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        size_t indent = 0;
        while (indent < line.size() && line[indent] == '\t') {
            ++indent;
        }
        if (indent == 0 && !std::isxdigit(static_cast<unsigned char>(line[0]))) {
            // Top-level sections that don't start with a hex vendor ID
            // (e.g. "C 0c  Serial bus controller") mark the start of the
            // device-class list at the end of the file; nothing past
            // this point is a vendor/device entry.
            break;
        }

        std::string rest = line.substr(indent);
        size_t sep = rest.find_first_of(" \t");
        if (sep == std::string::npos) {
            continue;
        }
        auto id = parse_hex_id(rest.substr(0, sep));
        if (!id) {
            continue;
        }
        std::string name = trim(rest.substr(sep));

        if (indent == 0) {
            current_vendor = *id;
            tables.vendors[current_vendor] = name;
        } else if (indent == 1) {
            tables.devices[composite_key(current_vendor, *id)] = name;
        }
        // indent >= 2: subsystem entries, not needed.
    }
    return tables;
}

template <size_t N>
IdTables load_first_available(const std::array<const char*, N>& candidates) {
    for (const char* path : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(path, ec) && !ec) {
            return parse_ids_file(path);
        }
    }
    return {};
}

} // namespace

std::optional<uint16_t> parse_hex_id(const std::string& s) {
    std::string digits = s;
    if (digits.size() > 2 && (digits[0] == '0') && (digits[1] == 'x' || digits[1] == 'X')) {
        digits = digits.substr(2);
    }
    if (digits.empty()) {
        return std::nullopt;
    }
    uint16_t value = 0;
    auto [ptr, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), value, 16);
    if (ec != std::errc() || ptr != digits.data() + digits.size()) {
        return std::nullopt;
    }
    return value;
}

struct HwIdDb::Impl {
    IdTables pci;
    IdTables usb;
};

HwIdDb::HwIdDb() : impl_(std::make_unique<Impl>()) {
    impl_->pci = load_first_available(kPciIdsPaths);
    impl_->usb = load_first_available(kUsbIdsPaths);
}

const HwIdDb& HwIdDb::instance() {
    static const HwIdDb db;
    return db;
}

std::optional<std::string> HwIdDb::pci_device_name(uint16_t vendor, uint16_t device) const {
    auto it = impl_->pci.devices.find(composite_key(vendor, device));
    if (it == impl_->pci.devices.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<std::string> HwIdDb::pci_vendor_name(uint16_t vendor) const {
    auto it = impl_->pci.vendors.find(vendor);
    if (it == impl_->pci.vendors.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<std::string> HwIdDb::usb_device_name(uint16_t vendor, uint16_t product) const {
    auto it = impl_->usb.devices.find(composite_key(vendor, product));
    if (it == impl_->usb.devices.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<std::string> HwIdDb::usb_vendor_name(uint16_t vendor) const {
    auto it = impl_->usb.vendors.find(vendor);
    if (it == impl_->usb.vendors.end()) {
        return std::nullopt;
    }
    return it->second;
}

} // namespace devgraph
