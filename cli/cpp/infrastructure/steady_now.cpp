#include "infrastructure/steady_now.hpp"

steady_now::time_point steady_now::now() const {
    return std::chrono::steady_clock::now();
}
