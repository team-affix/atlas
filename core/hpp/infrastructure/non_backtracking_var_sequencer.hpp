#ifndef NON_BACKTRACKING_VAR_SEQUENCER_HPP
#define NON_BACKTRACKING_VAR_SEQUENCER_HPP

#include <cstdint>
#include "interfaces/i_var_sequencer.hpp"

struct non_backtracking_var_sequencer : i_var_sequencer {
    explicit non_backtracking_var_sequencer(uint32_t initial = 0);
    uint32_t next() override;
    uint32_t peek() const;
private:
    uint32_t counter_;
};

#endif
