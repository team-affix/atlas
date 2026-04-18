#include "../../test_utils.hpp"
#include "../hpp/solver_cli_interface.hpp"
#include "../hpp/ridge_command_handler.hpp"
#include "../hpp/horizon_command_handler.hpp"
#include <sstream>
#include <iostream>
#include <cassert>
#include <string>

// ============================================================
// Minimal concrete subclass used to expose protected interface.
// ============================================================

struct test_solver : solver_cli_interface {
    test_solver(const std::string& file, const std::string& goals_str)
        : solver_cli_interface(file, goals_str) {}

    // expose protected members for white-box assertions
    using solver_cli_interface::t;
    using solver_cli_interface::pool;
    using solver_cli_interface::seq;
    using solver_cli_interface::bm;
    using solver_cli_interface::norm;
    using solver_cli_interface::db;
    using solver_cli_interface::gl;

    void call_print_bindings() { print_bindings(); }

protected:
    bool advance() override { return false; }
};

// ============================================================
// A subclass whose advance() returns exactly one true then false,
// used to test print_bindings indirectly via state after construction.
// ============================================================

struct ridge_test_exposed : ridge_command_handler {
    ridge_test_exposed(const std::string& file,
                       const std::string& goals_str,
                       size_t max_res,
                       double ec,
                       uint64_t seed)
        : ridge_command_handler(file, goals_str, max_res, ec, seed) {}

    bool call_advance() { return advance(); }

    using solver_cli_interface::t;
    using solver_cli_interface::db;
    using solver_cli_interface::gl;
};

struct horizon_test_exposed : horizon_command_handler {
    horizon_test_exposed(const std::string& file,
                         const std::string& goals_str,
                         size_t max_res,
                         double ec,
                         uint64_t seed)
        : horizon_command_handler(file, goals_str, max_res, ec, seed) {}

    bool call_advance() { return advance(); }

    using solver_cli_interface::t;
    using solver_cli_interface::db;
    using solver_cli_interface::gl;
};

// ============================================================
// Helpers
// ============================================================

// Redirect cout to a string, run fn(), then restore.
static std::string capture_cout(std::function<void()> fn) {
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    fn();
    std::cout.rdbuf(old);
    return oss.str();
}

// ============================================================
// solver_cli_interface tests
// ============================================================

void test_solver_cli_interface_constructor_basic() {
    // Simplest possible DB: one ground fact, no goal variables.
    // cli/examples/eq/db.chc contains "(eq X X)."
    test_solver s("cli/examples/eq/db.chc", "(eq a a)");

    // db must have been populated
    assert(s.db.size() == 1);

    // gl must have exactly one goal
    assert(s.gl.size() == 1);
}

void test_solver_cli_interface_constructor_db_contents() {
    // ancestor DB has 7 clauses (5 facts + 2 rules)
    test_solver s("cli/examples/ancestor/db.chc", "(ancestor tom bob)");

    assert(s.db.size() == 7);

    // First 5 are ground facts (no body atoms)
    for (size_t i = 0; i < 5; ++i)
        assert(s.db[i].body.empty());

    // Last 2 are rules
    assert(s.db[5].body.size() == 1);
    assert(s.db[6].body.size() == 2);
}

void test_solver_cli_interface_constructor_goals_no_vars() {
    // Goal with no variables: var_idx_to_name must be empty.
    test_solver s("cli/examples/eq/db.chc", "(eq a a)");

    assert(s.gl.size() == 1);

    // The goal expression should be interned in pool:
    // (eq a a) → cons(eq, cons(a, cons(a, nil)))
    const expr* expected = s.pool.cons(
        s.pool.atom("eq"),
        s.pool.cons(
            s.pool.atom("a"),
            s.pool.cons(s.pool.atom("a"), s.pool.atom("nil"))
        )
    );
    assert(s.gl[0] == expected);
}

void test_solver_cli_interface_constructor_goals_with_vars() {
    // Goal with variables: var_idx_to_name should have one entry for X.
    test_solver s("cli/examples/reachability/db.chc", "(reach 0 X)");

    assert(s.gl.size() == 1);

    // var_idx_to_name is private but we can test via print_bindings output:
    // Before any solving, the var is unbound, so print_bindings should print "X = ?<idx>"
    // just check it doesn't crash and produces non-empty output.
    std::string out = capture_cout([&]() { s.call_print_bindings(); });
    assert(!out.empty());
    // The output must contain the variable name
    assert(out.find("X") != std::string::npos);
}

void test_solver_cli_interface_invert() {
    // The static invert function is exercised by construction.
    // We verify that var_idx_to_name is the exact inverse of var_name_to_idx
    // by checking round-trip consistency via print_bindings.
    // Goal "(reach X Y)": X and Y each appear in the output.
    test_solver s("cli/examples/reachability/db.chc", "(reach X Y)");

    assert(s.gl.size() == 1);

    std::string out = capture_cout([&]() { s.call_print_bindings(); });
    // Both X and Y must appear in the output (each on its own line).
    assert(out.find("X") != std::string::npos);
    assert(out.find("Y") != std::string::npos);
    // There must be exactly 2 lines (one per variable).
    size_t newlines = std::count(out.begin(), out.end(), '\n');
    assert(newlines == 2);
}

void test_solver_cli_interface_print_bindings_atom() {
    // After a single ground goal (no variables), print_bindings produces no output
    // because there are no named goal variables.
    test_solver s("cli/examples/eq/db.chc", "(eq a b)");

    std::string out = capture_cout([&]() { s.call_print_bindings(); });
    assert(out.empty());
}

void test_solver_cli_interface_print_bindings_unbound_var() {
    // Goal variables that are unbound print by looking up their name in var_idx_to_name.
    // An unbound var normalizes to itself, and expr_printer prints the registered name.
    // So "X = X" and "Y = Y" are expected (the variable names, not "?<idx>").
    test_solver s("cli/examples/reachability/db.chc", "(reach X Y)");

    std::string out = capture_cout([&]() { s.call_print_bindings(); });
    // Both variable names appear in the output
    assert(out.find("X") != std::string::npos);
    assert(out.find("Y") != std::string::npos);
    // No "?" prefix — goal variables are always in the name map
    assert(out.find('?') == std::string::npos);
}

void test_solver_cli_interface_bad_file() {
    // Constructor must throw if the CHC file doesn't exist.
    assert_throws(
        test_solver("cli/examples/nonexistent_file.chc", "(p x)"),
        const std::runtime_error&
    );
}

void test_solver_cli_interface_bad_goal() {
    // Constructor must throw if the goal string is syntactically invalid.
    assert_throws(
        test_solver("cli/examples/eq/db.chc", ":-"),
        const std::runtime_error&
    );
}

void test_solver_cli_interface_multiple_goals() {
    // A goals string with two atoms produces gl.size() == 2.
    test_solver s("cli/examples/reachability/db.chc", "(reach 0 X), (reach X Y)");

    assert(s.gl.size() == 2);

    // Both X and Y must appear in the variable output.
    std::string out = capture_cout([&]() { s.call_print_bindings(); });
    assert(out.find("X") != std::string::npos);
    assert(out.find("Y") != std::string::npos);
}

// ============================================================
// ridge_command_handler tests
// ============================================================

void test_ridge_command_handler_constructor() {
    // Basic construction must not throw.
    ridge_test_exposed h("cli/examples/eq/db.chc", "(eq a a)", 1000, 1.41, 42);

    // DB loaded correctly
    assert(h.db.size() == 1);
    // One goal
    assert(h.gl.size() == 1);
}

void test_ridge_command_handler_advance_sat() {
    // (eq X X). with goal (eq a a) is trivially satisfiable.
    // advance() must return true (found a solution).
    ridge_test_exposed h("cli/examples/eq/db.chc", "(eq a a)", 10000, 1.41, 0);
    bool solved = h.call_advance();
    assert(solved == true);
}

void test_ridge_command_handler_advance_unsat() {
    // (eq X X). — ground fact: eq(X,X).
    // Goal (eq a b): a ≠ b, so it's unsatisfiable; advance() returns false.
    ridge_test_exposed h("cli/examples/eq/db.chc", "(eq a b)", 10000, 1.41, 0);
    bool solved = h.call_advance();
    assert(solved == false);
}

void test_ridge_command_handler_advance_ancestor() {
    // ancestor DB: goal (ancestor tom bob) is satisfiable in one step.
    ridge_test_exposed h("cli/examples/ancestor/db.chc", "(ancestor tom bob)", 10000, 1.41, 0);
    bool solved = h.call_advance();
    assert(solved == true);
}

void test_ridge_command_handler_advance_unsat_ancestor() {
    // bob is not an ancestor of tom in the family tree — unsatisfiable.
    ridge_test_exposed h("cli/examples/ancestor/db.chc", "(ancestor bob tom)", 10000, 1.41, 0);
    bool solved = h.call_advance();
    assert(solved == false);
}

void test_ridge_command_handler_seed_determinism() {
    // Same seed → same result for a given problem.
    {
        ridge_test_exposed h1("cli/examples/eq/db.chc", "(eq a a)", 1000, 1.41, 12345);
        ridge_test_exposed h2("cli/examples/eq/db.chc", "(eq a a)", 1000, 1.41, 12345);
        assert(h1.call_advance() == h2.call_advance());
    }
    {
        ridge_test_exposed h1("cli/examples/eq/db.chc", "(eq a b)", 1000, 1.41, 99);
        ridge_test_exposed h2("cli/examples/eq/db.chc", "(eq a b)", 1000, 1.41, 99);
        assert(h1.call_advance() == h2.call_advance());
    }
}

// ============================================================
// horizon_command_handler tests
// ============================================================

void test_horizon_command_handler_constructor() {
    // Basic construction must not throw.
    horizon_test_exposed h("cli/examples/eq/db.chc", "(eq a a)", 1000, 1.41, 42);

    assert(h.db.size() == 1);
    assert(h.gl.size() == 1);
}

void test_horizon_command_handler_advance_sat() {
    // Same trivial satisfiable case as ridge.
    horizon_test_exposed h("cli/examples/eq/db.chc", "(eq a a)", 10000, 1.41, 0);
    bool solved = h.call_advance();
    assert(solved == true);
}

void test_horizon_command_handler_advance_unsat() {
    horizon_test_exposed h("cli/examples/eq/db.chc", "(eq a b)", 10000, 1.41, 0);
    bool solved = h.call_advance();
    assert(solved == false);
}

void test_horizon_command_handler_advance_ancestor() {
    horizon_test_exposed h("cli/examples/ancestor/db.chc", "(ancestor tom bob)", 10000, 1.41, 0);
    bool solved = h.call_advance();
    assert(solved == true);
}

void test_horizon_command_handler_advance_unsat_ancestor() {
    horizon_test_exposed h("cli/examples/ancestor/db.chc", "(ancestor bob tom)", 10000, 1.41, 0);
    bool solved = h.call_advance();
    assert(solved == false);
}

void test_horizon_command_handler_seed_determinism() {
    {
        horizon_test_exposed h1("cli/examples/eq/db.chc", "(eq a a)", 1000, 1.41, 12345);
        horizon_test_exposed h2("cli/examples/eq/db.chc", "(eq a a)", 1000, 1.41, 12345);
        assert(h1.call_advance() == h2.call_advance());
    }
    {
        horizon_test_exposed h1("cli/examples/eq/db.chc", "(eq a b)", 1000, 1.41, 99);
        horizon_test_exposed h2("cli/examples/eq/db.chc", "(eq a b)", 1000, 1.41, 99);
        assert(h1.call_advance() == h2.call_advance());
    }
}

// ============================================================
// Test harness
// ============================================================

void unit_test_main() {
    constexpr bool ENABLE_DEBUG_LOGS = true;

    // solver_cli_interface
    TEST(test_solver_cli_interface_constructor_basic);
    TEST(test_solver_cli_interface_constructor_db_contents);
    TEST(test_solver_cli_interface_constructor_goals_no_vars);
    TEST(test_solver_cli_interface_constructor_goals_with_vars);
    TEST(test_solver_cli_interface_invert);
    TEST(test_solver_cli_interface_print_bindings_atom);
    TEST(test_solver_cli_interface_print_bindings_unbound_var);
    TEST(test_solver_cli_interface_bad_file);
    TEST(test_solver_cli_interface_bad_goal);
    TEST(test_solver_cli_interface_multiple_goals);

    // ridge_command_handler
    TEST(test_ridge_command_handler_constructor);
    TEST(test_ridge_command_handler_advance_sat);
    TEST(test_ridge_command_handler_advance_unsat);
    TEST(test_ridge_command_handler_advance_ancestor);
    TEST(test_ridge_command_handler_advance_unsat_ancestor);
    TEST(test_ridge_command_handler_seed_determinism);

    // horizon_command_handler
    TEST(test_horizon_command_handler_constructor);
    TEST(test_horizon_command_handler_advance_sat);
    TEST(test_horizon_command_handler_advance_unsat);
    TEST(test_horizon_command_handler_advance_ancestor);
    TEST(test_horizon_command_handler_advance_unsat_ancestor);
    TEST(test_horizon_command_handler_seed_determinism);
}

int main() {
    unit_test_main();
    return 0;
}
