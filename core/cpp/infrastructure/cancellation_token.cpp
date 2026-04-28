#include "../../hpp/infrastructure/cancellation_token.hpp"

cancellation_token::cancellation_token() : cancelled(false) {
}

void cancellation_token::cancel() {
    cancelled = true;
}

void cancellation_token::reset() {
    cancelled = false;
}

bool cancellation_token::is_cancelled() const {
    return cancelled;
}
