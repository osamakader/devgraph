#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace devgraph {

// Resolves PCI/USB vendor:device IDs to human-readable names using the
// system's local pci.ids/usb.ids database (shipped by the "hwdata" or
// "pciutils"/"usbutils" packages on essentially every distro). Devices
// like "0000:00:16.0" are otherwise just a bus address -- this is what
// turns that into "Intel Corporation ... HECI Controller".
//
// If neither database can be found on disk, lookups simply return
// nullopt; nothing in the tool depends on this data being present.
class HwIdDb {
public:
    static const HwIdDb& instance();

    std::optional<std::string> pci_device_name(uint16_t vendor, uint16_t device) const;
    std::optional<std::string> pci_vendor_name(uint16_t vendor) const;

    std::optional<std::string> usb_device_name(uint16_t vendor, uint16_t product) const;
    std::optional<std::string> usb_vendor_name(uint16_t vendor) const;

private:
    HwIdDb();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Parses a "0x8086" or "8086"-style hex string. Returns nullopt if the
// string isn't valid hex (e.g. the sysfs attribute was missing/empty).
std::optional<uint16_t> parse_hex_id(const std::string& s);

} // namespace devgraph
