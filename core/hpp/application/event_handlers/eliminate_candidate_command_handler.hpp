#ifndef ELIMINATE_CANDIDATE_EVENT_HANDLER_HPP
#define ELIMINATE_CANDIDATE_EVENT_HANDLER_HPP

#include "../../../infrastructure/command_handler.hpp"
#include "../../commands/eliminate_candidate_command.hpp"
#include "candidate_store.hpp"

struct eliminate_candidate_command_handler : command_handler<eliminate_candidate_command> {
    eliminate_candidate_command_handler();
    void operator()(const eliminate_candidate_command& c) override;
#ifndef DEBUG
private:
#endif
    candidate_store& cs;
};

#endif
