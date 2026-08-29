#include "infrastructure/stdin_idle.hpp"
#include <poll.h>
#include <unistd.h>

void stdin_idle::idle() {
    constexpr int k_idle_timeout_ms = 200;
    pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    pfd.revents = 0;
    poll(&pfd, 1, k_idle_timeout_ms);
}
