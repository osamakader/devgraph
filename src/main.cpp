#include "filter.hpp"
#include "render_dot.hpp"
#include "render_list.hpp"
#include "render_options.hpp"
#include "render_tree.hpp"
#include "sysfs_scanner.hpp"
#include "util.hpp"

#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

using namespace devgraph;

enum class Format { Tree, List, Dot };

struct CliOptions {
    std::filesystem::path root = "/sys/devices";
    Format format = Format::Tree;
    FilterOptions filter;
    RenderOptions render;
    bool no_devicetree = false;
};

void print_usage(const char* argv0) {
    std::cout <<
        "devgraph - map the live Linux hardware topology\n"
        "\n"
        "Builds a device graph from /sys/devices, the device tree, /dev,\n"
        "and driver bindings, and renders the I2C/SPI/USB/PCI/platform\n"
        "hierarchy as a tree, flat list, or Graphviz graph.\n"
        "\n"
        "Usage: " << argv0 << " [options]\n"
        "\n"
        "Options:\n"
        "  -r, --root PATH        Start at PATH instead of /sys/devices\n"
        "                          (e.g. /sys/devices/platform/soc)\n"
        "  -f, --format FORMAT    Output format: tree (default), list, dot\n"
        "  -s, --subsystem LIST   Only show devices on these buses\n"
        "                          (comma-separated, e.g. i2c,spi,usb,pci)\n"
        "  -d, --driver NAME      Only show devices bound to driver NAME\n"
        "      --with-devnode     Only show devices that have a /dev node\n"
        "      --no-devicetree    Skip merging in device-tree-only nodes\n"
        "      --no-driver        Hide driver annotations\n"
        "      --no-devnode       Hide /dev node annotations\n"
        "      --no-compatible    Hide device-tree compatible annotations\n"
        "      --no-color         Disable ANSI colors\n"
        "  -h, --help             Show this help\n"
        "\n"
        "Ancestors of a matching device are always kept when filtering,\n"
        "so the surrounding bus topology stays visible.\n";
}

std::optional<CliOptions> parse_args(int argc, char** argv) {
    CliOptions opts;
    opts.render.use_color = util::stdout_is_tty();

    std::vector<std::string> args(argv + 1, argv + argc);
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        auto next_value = [&](const char* flag) -> std::string {
            if (i + 1 >= args.size()) {
                std::cerr << "devgraph: " << flag << " requires an argument\n";
                std::exit(2);
            }
            return args[++i];
        };

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        } else if (arg == "-r" || arg == "--root") {
            opts.root = next_value(arg.c_str());
        } else if (arg == "-f" || arg == "--format") {
            std::string v = next_value(arg.c_str());
            if (v == "tree") {
                opts.format = Format::Tree;
            } else if (v == "list") {
                opts.format = Format::List;
            } else if (v == "dot") {
                opts.format = Format::Dot;
            } else {
                std::cerr << "devgraph: unknown format '" << v << "' (expected tree, list, dot)\n";
                return std::nullopt;
            }
        } else if (arg == "-s" || arg == "--subsystem") {
            opts.filter.subsystems = util::split(next_value(arg.c_str()), ',');
        } else if (arg == "-d" || arg == "--driver") {
            opts.filter.driver = next_value(arg.c_str());
        } else if (arg == "--with-devnode") {
            opts.filter.only_with_devnode = true;
        } else if (arg == "--no-devicetree") {
            opts.no_devicetree = true;
        } else if (arg == "--no-driver") {
            opts.render.show_driver = false;
        } else if (arg == "--no-devnode") {
            opts.render.show_devnode = false;
        } else if (arg == "--no-compatible") {
            opts.render.show_compatible = false;
        } else if (arg == "--no-color") {
            opts.render.use_color = false;
        } else {
            std::cerr << "devgraph: unknown option '" << arg << "'\n";
            return std::nullopt;
        }
    }
    return opts;
}

} // namespace

int main(int argc, char** argv) {
    auto parsed = parse_args(argc, argv);
    if (!parsed) {
        print_usage(argv[0]);
        return 2;
    }
    CliOptions opts = *parsed;

    std::error_code ec;
    if (!std::filesystem::is_directory(opts.root, ec) || ec) {
        std::cerr << "devgraph: root path '" << opts.root.string() << "' is not a directory\n";
        return 1;
    }

    ScanOptions scan_opts;
    scan_opts.sysfs_root = opts.root;
    scan_opts.include_devicetree = !opts.no_devicetree;

    ScanResult result = scan(scan_opts);
    apply_filter(result.root.get(), opts.filter);

    switch (opts.format) {
        case Format::Tree:
            render::render_tree(std::cout, *result.root, opts.render);
            break;
        case Format::List:
            render::render_list(std::cout, *result.root, opts.render);
            break;
        case Format::Dot:
            render::render_dot(std::cout, *result.root, opts.render);
            break;
    }

    return 0;
}
