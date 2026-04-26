#include "../../../../hpp/domain/entities/candidate_store/eliminate_candidate_command_handler.hpp"
#include "../../../../hpp/infrastructure/locator.hpp"

eliminate_candidate_command_handler::eliminate_candidate_command_handler() :
    cs(locator::locate<candidate_store>()) {
}

void eliminate_candidate_command_handler::operator()(const eliminate_candidate_command& c) {
    auto gl = c.rl->parent;
    auto idx = c.rl->idx;
    cs.at(gl).erase(idx);
}
