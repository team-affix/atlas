#ifndef VAR_SEQUENCER_HPP
#define VAR_SEQUENCER_HPP

#include "../domain/interfaces/i_var_sequencer.hpp"
#include "../utility/sequencer.hpp"

struct var_sequencer : i_var_sequencer {
    var_sequencer();
    uint32_t next() override;
private:
    sequencer<uint32_t> seq;
};

#endif
