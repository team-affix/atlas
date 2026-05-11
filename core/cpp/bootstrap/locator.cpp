#include "../../hpp/bootstrap/locator.hpp"

const bindings* locator::b = nullptr;

void locator::register_bindings(const bindings* bndgs) {
    locator::b = bndgs;
}
