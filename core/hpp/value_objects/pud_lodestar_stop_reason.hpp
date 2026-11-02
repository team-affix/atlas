#ifndef PUD_LODESTAR_STOP_REASON_HPP
#define PUD_LODESTAR_STOP_REASON_HPP

#include <cstdint>

// Why a Lodestar session stopped ticking. refuted: no open conjecture leaf is left. exhausted: open
// conjecture leaves remain but the session's node limits forbid every unfold. budget_spent: the
// session made its maximum number of unfolds.
enum struct pud_lodestar_stop_reason : uint8_t {
    running,
    refuted,
    exhausted,
    budget_spent,
};

#endif
