#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <vector>

// Minimal self-registering test framework: no external dependencies, so
// the build doesn't need network access to fetch a framework. Each
// DEVGRAPH_TEST(name) { ... } block registers itself at static-init time;
// test_main.cpp runs the full registry and reports pass/fail per test.

namespace devgraph::test {

struct Failure {
    std::string expr;
    const char* file;
    int line;
};

class TestCase {
public:
    TestCase(std::string name, std::function<void()> fn) : name_(std::move(name)), fn_(std::move(fn)) {
        registry().push_back(this);
    }

    static std::vector<TestCase*>& registry() {
        static std::vector<TestCase*> instances;
        return instances;
    }

    const std::string& name() const { return name_; }
    void run() const { fn_(); }

private:
    std::string name_;
    std::function<void()> fn_;
};

inline std::vector<Failure>& current_failures() {
    static std::vector<Failure> failures;
    return failures;
}

inline void record_failure(std::string expr, const char* file, int line) {
    current_failures().push_back({std::move(expr), file, line});
}

} // namespace devgraph::test

#define DEVGRAPH_TEST(name)                                                                                          \
    static void devgraph_test_body_##name();                                                                         \
    static ::devgraph::test::TestCase devgraph_test_case_##name(#name, devgraph_test_body_##name);                   \
    static void devgraph_test_body_##name()

#define CHECK(cond)                                                                                                  \
    do {                                                                                                             \
        if (!(cond)) {                                                                                               \
            ::devgraph::test::record_failure(#cond, __FILE__, __LINE__);                                             \
        }                                                                                                            \
    } while (0)

#define CHECK_EQ(a, b)                                                                                               \
    do {                                                                                                             \
        auto&& _devgraph_a = (a);                                                                                    \
        auto&& _devgraph_b = (b);                                                                                    \
        if (!(_devgraph_a == _devgraph_b)) {                                                                         \
            std::ostringstream _devgraph_ss;                                                                         \
            _devgraph_ss << #a << " == " << #b << "  (got \"" << _devgraph_a << "\", expected \"" << _devgraph_b     \
                          << "\")";                                                                                  \
            ::devgraph::test::record_failure(_devgraph_ss.str(), __FILE__, __LINE__);                                \
        }                                                                                                            \
    } while (0)
