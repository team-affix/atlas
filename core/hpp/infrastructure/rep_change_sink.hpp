#ifndef REP_CHANGE_SINK_HPP
#define REP_CHANGE_SINK_HPP

#include <queue>
#include "../domain/interfaces/i_rep_change_sink.hpp"

struct rep_change_sink : i_rep_change_sink {
    virtual ~rep_change_sink() = default;
    void push(uint32_t) override;
    uint32_t pop() override;
    bool empty() const override;
private:
    std::queue<uint32_t> q_;
};

#endif
