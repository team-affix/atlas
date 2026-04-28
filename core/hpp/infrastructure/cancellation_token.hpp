#ifndef CANCELLATION_TOKEN_HPP
#define CANCELLATION_TOKEN_HPP

#include "task.hpp"

struct cancellation_token : task {
    cancellation_token();
    void cancel();
    void reset();
    bool is_cancelled() const;
#ifndef DEBUG
private:
#endif
    bool cancelled;
};

#endif
