#ifndef CDCL_SEQUENCER_HPP
#define CDCL_SEQUENCER_HPP

#include "../domain/interfaces/i_cdcl_sequencer.hpp"
#include "../utility/sequencer.hpp"

struct cdcl_sequencer : i_cdcl_sequencer {
    cdcl_sequencer();
    size_t next() override;
private:
    sequencer seq;
};

#endif
