#include "../../../doctest/doctest/doctest.h"
#include "../../../core/hpp/infrastructure/overlay_bind_map.hpp"
#include "../../../core/hpp/infrastructure/bind_map.hpp"
#include "../../../core/hpp/bootstrap/bindings.hpp"
#include "../../../core/hpp/bootstrap/locator.hpp"
#include "../../../core/hpp/domain/interfaces/i_factory.hpp"

namespace {
    struct bind_map_factory : i_factory<i_bind_map> {
        std::unique_ptr<i_bind_map> make() override {
            return std::make_unique<bind_map>();
        }
    };
}

TEST_CASE("overlay_bind_map") {
    bindings b;
    bind_map_factory factory;
    b.bind<i_factory<i_bind_map>>(factory);
    locator::register_bindings(&b);

    bind_map remote;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr func{expr::functor{"f", {}}};
    expr func2{expr::functor{"g", {}}};

    overlay_bind_map obm(remote);

    SUBCASE("bind goes to local; remote is unchanged") {
        obm.bind(0, &func);
        CHECK(remote.whnf(&var0) == &var0);
    }

    SUBCASE("whnf resolves from local binding") {
        obm.bind(0, &func);
        CHECK(obm.whnf(&var0) == &func);
    }

    SUBCASE("whnf falls through to remote when no local binding") {
        remote.bind(0, &func);
        CHECK(obm.whnf(&var0) == &func);
    }

    SUBCASE("whnf chain: local var0->var1, remote var1->functor") {
        obm.bind(0, &var1);
        remote.bind(1, &func);
        CHECK(obm.whnf(&var0) == &func);
    }

    SUBCASE("two overlays on same remote are independent") {
        overlay_bind_map obm2(remote);
        obm.bind(0, &func);
        obm2.bind(0, &func2);
        CHECK(obm.whnf(&var0) == &func);
        CHECK(obm2.whnf(&var0) == &func2);
    }

    SUBCASE("local binding shadows remote binding for the same var") {
        remote.bind(0, &func2);
        obm.bind(0, &func);
        CHECK(obm.whnf(&var0) == &func);
    }

    SUBCASE("functor passthrough") {
        CHECK(obm.whnf(&func) == &func);
    }

    locator::register_bindings(nullptr);
}
