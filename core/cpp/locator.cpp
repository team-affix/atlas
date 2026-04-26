#include "../hpp/locator.hpp"

void locator::unbind(locator_keys key) {
    entries.erase(key);
}

void locator::purge() {
    entries.clear();
}
