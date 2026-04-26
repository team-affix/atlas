#include "../hpp/infrastructure/locator.hpp"

void locator::push_frame() {
    past_frames.push(std::move(current_frame_additions));
    current_frame_additions.clear();
}

void locator::pop_frame() {
    clear_frame();
    current_frame_additions = std::move(past_frames.top());
    past_frames.pop();
}

void locator::clear_frame() {
    for (auto type : current_frame_additions)
        entries.erase(type);
    current_frame_additions.clear();
}