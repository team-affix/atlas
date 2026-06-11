#ifndef RUNTIME_TEST_HARNESS_HPP
#define RUNTIME_TEST_HARNESS_HPP

#include <functional>
#include <iostream>
#include <set>
#include <vector>
#include <gtest/gtest.h>
#include "interfaces/i_expr_printer.hpp"
#include "interfaces/i_runtime.hpp"
#include "value_objects/expr.hpp"

using solution = std::vector<const expr*>;

inline void print_solution(
    size_t solution_index, i_expr_printer& printer, const solution& s) {
    std::cout << "solution " << solution_index << '\n';
    for (const expr* e : s) {
        std::cout << "  ";
        printer.print(e);
        std::cout << '\n';
    }
}

inline void enumerate_all_solutions(
    i_runtime& session,
    i_expr_printer& printer,
    std::set<solution> expected,
    const std::function<solution()>& get_solution) {
    std::set<solution> visited;
    size_t solution_index = 0;
    while (!expected.empty()) {
        if (!session.next()) {
            std::cout << "solver stopped early; expected_remaining=" << expected.size()
                      << "visited=" << visited.size() << '\n';
            for (const solution& s : expected)
                print_solution(solution_index++, printer, s);
            FAIL() << "solver stopped before all expected solutions found";
        }
        if (!session.solved())
            continue;
        const solution s = get_solution();
        if (visited.count(s))
            continue;
        print_solution(solution_index, printer, s);
        auto it = expected.find(s);
        if (it == expected.end()) {
            std::cout << "unexpected solution (not in expected set)\n";
            FAIL() << "unexpected solution";
        }
        expected.erase(it);
        visited.insert(s);
        ++solution_index;
    }
}

inline void next_until_refuted(
    i_runtime& session,
    i_expr_printer& printer,
    std::set<solution> expected,
    const std::function<solution()>& get_solution) {
    std::set<solution> visited;
    size_t solution_index = 0;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = get_solution();
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        if (it == expected.end()) {
            std::cout << "unexpected solution (not in expected set)\n";
            print_solution(solution_index, printer, s);
            FAIL() << "unexpected solution";
        }
        expected.erase(it);
        ++solution_index;
    }
    if (!expected.empty()) {
        std::cout << "solver refuted early; expected_remaining=" << expected.size()
                  << " visited=" << visited.size() << '\n';
        for (const solution& s : visited)
            print_solution(solution_index++, printer, s);
        FAIL() << "solver refuted before all expected solutions found";
    }
}

#endif
