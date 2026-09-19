#include "framework.hpp"

#include <iostream>

int main() {
    using namespace devgraph::test;

    int failed_tests = 0;
    for (const auto* tc : TestCase::registry()) {
        current_failures().clear();
        tc->run();
        if (current_failures().empty()) {
            std::cout << "[ OK ] " << tc->name() << "\n";
        } else {
            ++failed_tests;
            std::cout << "[FAIL] " << tc->name() << "\n";
            for (const auto& f : current_failures()) {
                std::cout << "    " << f.file << ":" << f.line << ": " << f.expr << "\n";
            }
        }
    }

    int total = static_cast<int>(TestCase::registry().size());
    std::cout << (total - failed_tests) << "/" << total << " tests passed\n";
    return failed_tests == 0 ? 0 : 1;
}
