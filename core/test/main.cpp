#define DOCTEST_CONFIG_IMPLEMENT
#include "../../doctest/doctest/doctest.h"

int main(int argc, char** argv) {
    doctest::Context ctx(argc, argv);
    ctx.setOption("success", true);
    ctx.setOption("duration", true);
    return ctx.run();
}
