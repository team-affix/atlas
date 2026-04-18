# AI Prompt Memory - Important Rules and Guidelines

## File Creation

- **NEVER create files without explicit permission**
- Do not create documentation files, bug reports, or any other files "willy nilly"
- Only create files when the user explicitly asks for them

## Testing Guidelines

- Go one function at a time when testing
- **Every test case must be fully resource-independent:** each `{}` block creates its own local variables (trail, expr_pool, database, lineage_pool, etc.). Never share resources between test cases within a test function.
- After writing each test, RUN make, then RUN the program (./main)
- Use the TEST() macro from test_utils.hpp - do NOT add "all tests passed" print statements
- Tests should use assert() extensively to verify behavior
- Think of corner cases that could cause functions to fail
- Do NOT create separate test cases for edge cases that should be covered in the main tests
- Test functions in the SAME ORDER as they are declared in the header file
- **Strong foundation rule: the FIRST TIME any method is used in the test suite must be inside its OWN test function.** Never use a method as a setup primitive in an earlier test function before it has been formally tested. If a later test needs a method as a prerequisite, reorder the tests so the prerequisite is tested first. Verify state using internal member access (e.g. `f.members`) in DEBUG mode rather than calling untested frontier API methods.

## Code Style

- Users construct expressions naturally, NOT with explicit variant construction:
  - Use: `expr e1{atom{"test"}}`
  - NOT: `expr e1{std::variant<atom, cons, var>{atom{"test"}}}`
- Use inline construction for deeply nested structures:
  ```cpp
  expr e{cons{
      expr{cons{expr{atom{"a"}}, expr{atom{"b"}}}},
      expr{var{1}}
  }};
  ```

## Design Principles

- All expression structures (atom, var, cons, expr) are immutable once constructed
- cons pointers (m_lhs, m_rhs) should always be non-null due to the restrictive interface
- The interface only allows construction with references, not nullable pointers
- Moved-from cons objects are invalid and should not be used (standard C++ behavior)

## Test Coverage and Completeness

- **100% method coverage:** every public (and protected-in-DEBUG) method must have at least one dedicated test function.
- **100% functionality coverage:** every distinct behavior, branch, and edge case of each method must be covered by an assertion somewhere in the suite. A method that has multiple logical paths needs test cases that exercise all of them.
- **Hardcore testing:** do not write token tests that merely call a method and ignore the result. Every test must assert concrete, specific values — sizes, member states, return values, side effects on collaborators.
- **Contiguous grouping:** all test functions for the same class must be declared together in one uninterrupted block. Never interleave tests from different classes.
- **Declaration order = invocation order:** the order in which test functions are defined in the file must exactly match the order in which they are called in `unit_test_main`. No exceptions.
- **Topological sort:** classes that depend on other classes must have their tests appear after the tests of their dependencies (strong foundation rule). Within a class block, test simpler/foundational methods before methods that build on them.

## Bug Finding and Testing

- Actively think about common bugs that happen with certain function types:
  - Copy constructors: self-assignment, deep vs shallow copy, resource leaks
  - Move constructors: self-move, leaving valid state, double-free
  - Assignment operators: self-assignment, resource cleanup before assignment
  - Destructors: double-free, resource leaks
  - Getters with pointers: nullptr dereference
  - Functions with unique_ptr: moved-from state, nullptr handling
  - Any other code pattern that might present known issues (etc.)
- Consider the architecture of the program and test for architectural bugs
- When a test throws an error:
  - Analyze why the error was thrown
  - If you suspect it was due to the test case being written wrong, fix the test case automatically
  - If a bug in the core source code is highly likely, **STOP TESTING IMMEDIATELY** and report it to the user
- **DO NOT CORRECT BUGS IN CORE SOURCE CODE** - report them to the user and let them fix it

### Assertive Testing Philosophy

- **Don't be afraid to assert what you think should be true, even if you're unsure**
- If you have an idea about what the state of something should be mid-test, make an assertion for it
- When an assertion fails:
  1. Check if the failure makes sense given the system's actual behavior
  2. If the behavior is correct, adjust the assertion to match reality
  3. This process is VALUABLE because it forces you to confront and justify the actual behavior
- **Benefits of this approach:**
  - Forces deep understanding of the system by vetting assumptions
  - Even "wrong" initial assertions lead to correct final assertions
  - The corrected assertions serve as regression tests for the future, and they are hard fought and won assertions
  - Comprehensive assertions catch bugs that might otherwise go unnoticed
- **Example:** When testing `expr_pool` with nested frames, assert `pool.exprs.size()` after every operation, even if you're not 100% sure what the size should be. If it fails, investigate why, then fix the assertion to match the correct behavior.

## Building and Debugging

- To build for testing, use the fast debug variants: `make core_debug_fast`, `make cli_debug_fast`, etc.
- To run the test binary: `./build/core_debug_fast`
- Always prefer the `_fast` targets over plain debug targets to avoid timeouts during iterative testing.
- The fast variants compile with `-O3` and `-DDEBUG`, giving full member access for assertions while still running quickly.

## Communication

- Do not be overly verbose or use unnecessary superlatives
- Get straight to the point
- When asked to find bugs, analyze the code thoroughly and report findings concisely
