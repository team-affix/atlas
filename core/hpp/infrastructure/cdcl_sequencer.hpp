#ifndef CDCL_SEQUENCER_HPP
#define CDCL_SEQUENCER_HPP

#include "interfaces/i_cdcl_sequencer.hpp"
#include "interfaces/i_trail.hpp"
#include "infrastructure/sequencer.hpp"

struct cdcl_sequencer : i_cdcl_sequencer {
    cdcl_sequencer(i_trail& t);
    size_t next() override;
private:
    sequencer<size_t> seq;
};

#endif
