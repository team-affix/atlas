#include "../../hpp/bootstrap/resolver.hpp"

const bindings* resolver::b = nullptr;

void resolver::register_bindings(const bindings* bndgs) {
    resolver::b = bndgs;
}
