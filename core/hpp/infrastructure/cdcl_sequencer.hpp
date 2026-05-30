#ifndef CDCL_SEQUENCER_HPP
#define CDCL_SEQUENCER_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_cdcl_sequencer.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"
#include "infrastructure/sequencer.hpp"

struct cdcl_sequencer : i_cdcl_sequencer {
    cdcl_sequencer(locator& loc);
    size_t next() override;
private:
    sequencer<size_t> seq;
};

#endif
