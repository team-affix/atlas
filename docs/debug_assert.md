# DEBUG_ASSERT

`DEBUG_ASSERT` is defined in [`debug_assert.hpp`](../debug_assert.hpp).

| Build | Expansion |
|-------|-----------|
| Debug (`-DDEBUG`) | `if (!(cond)) throw std::logic_error(#cond);` |
| Release | `((void)0)` — **`cond` is not evaluated** |

## Purpose

Use `DEBUG_ASSERT` to check **invariants** on paths that tests already prove are correct in production. Debug builds and `EXPECT_THROW` tests document misuse; release builds assume callers never violate the contract.

It is **not** a substitute for error handling on paths that can fail at runtime.

## Do not put side effects inside `DEBUG_ASSERT`

Because release strips the entire macro argument, any function call inside `DEBUG_ASSERT(...)` **does not run in release**.

**Wrong** — `tree_.insert` only runs in debug builds:

```cpp
auto [_, inserted] = in_flight_.emplace(gl);
DEBUG_ASSERT(inserted);
DEBUG_ASSERT(tree_.insert(gl));  // bug: insert elided in release
```

That pattern broke ridge in release: goals were tracked in `in_flight_` but never inserted into the SRT tree, so `empty()` was always true and the solver reported vacuous `SOLVED` ticks with unbound variables.

**Right** — perform the effect, then assert on the result:

```cpp
auto [_, inserted] = in_flight_.emplace(gl);
DEBUG_ASSERT(inserted);
const bool tree_inserted = tree_.insert(gl);
DEBUG_ASSERT(tree_inserted);
```

Same rule for any mutating call: `insert`, `erase`, `emplace`, `push`, assignments, etc.

## Safe uses

These are fine — the work happens **before** the assert; `DEBUG_ASSERT` only inspects an existing value:

```cpp
auto [_, inserted] = map.emplace(key, value);
DEBUG_ASSERT(inserted);

const auto erased = map.erase(key);
DEBUG_ASSERT(erased == 1);

DEBUG_ASSERT(!map.empty());
DEBUG_ASSERT(it != map.end());  // after a prior lookup stored in it
```

Read-only queries inside `DEBUG_ASSERT` (e.g. `contains`, `find` compared to `end`) are technically safe in release because eliding them does not skip required mutation — but prefer asserting on a variable you already computed if the query is expensive or non-obvious.

## Review checklist

When you see `DEBUG_ASSERT(` in a diff:

1. Does the argument **call a function that mutates state**? Split it out.
2. Does the argument **compute a value that production code needs**? Store it first, then assert.
3. Is failure possible on a valid production path? Use real error handling, not `DEBUG_ASSERT`.
