#ifndef PUD_SEARCH_STOP_REASON_HPP
#define PUD_SEARCH_STOP_REASON_HPP

#include <cstdint>

// Why a PUD search session stopped ticking. refuted: no open focus leaf is left. exhausted: open
// focus leaves remain but the session's node limits forbid every unfold. budget_spent: the
// session made its maximum number of unfolds. disk_full: the session's module files reached their disk budget.
enum struct pud_search_stop_reason : uint8_t {
    running,
    refuted,
    exhausted,
    budget_spent,
    disk_full,
};

#endif
