#ifndef CDCL_SEQUENCER_HPP
#define CDCL_SEQUENCER_HPP

#include "../interfaces/i_cdcl_sequencer.hpp"
#include "../utility/sequencer.hpp"

struct cdcl_sequencer : i_cdcl_sequencer {
    cdcl_sequencer();
    size_t next() override;
private:
    sequencer<size_t> seq;
};

#endif
