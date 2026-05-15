#include "../../../doctest/doctest/doctest.h"
#include "../../../core/hpp/infrastructure/overlay_bind_map.hpp"
#include "../../../core/hpp/bootstrap/bindings.hpp"
#include "../../../core/hpp/bootstrap/locator.hpp"
#include "../../../core/hpp/domain/interfaces/i_factory.hpp"
#include <unordered_map>
#include <variant>
#include <vector>

namespace staged {
    struct staged_bind_map_factory_make {
        std::unique_ptr<i_bind_map> output;
        std::monostate input;
    };
    struct staged_bind_map_bind {
        std::monostate output;
        std::tuple<uint32_t, const expr*> input;
    };
    struct staged_bind_map_whnf {
        const expr* output;
        const expr* input;
    };
}

namespace test_helpers {
    template<typename... Alternatives>
    struct staged_invocation_sequence {
        using staged_t = std::variant<Alternatives...>;
        staged_invocation_sequence(std::vector<staged_t> staged) : staged(std::move(staged)) {}
        template<typename Invoc, typename Input, typename Response>
        Response invoke(Input input) {
            auto& e = staged.at(index++);
            auto& v = std::get<Invoc>(e);
            assert(v.input == input);
            return std::move(v.output);
        }
    private:
        std::vector<staged_t> staged;
        size_t index = 0;
    };
}

namespace {
    struct mock_bind_map;

    struct bind_call { uint32_t index; const expr* e; };
    struct whnf_call { const expr* input; };

    struct call_record {
        mock_bind_map*                       source;
        std::variant<bind_call, whnf_call>   call;
    };

    struct mock_bind_map : i_bind_map {
        mock_bind_map() : log(locator::locate<std::vector<call_record>>()) {}

        std::unordered_map<const expr*, const expr*> whnf_responses;

        void bind(uint32_t index, const expr* e) override {
            log.push_back({this, bind_call{index, e}});
        }

        const expr* whnf(const expr* e) override {
            log.push_back({this, whnf_call{e}});
            auto it = whnf_responses.find(e);
            return it != whnf_responses.end() ? it->second : e;
        }
    private:
        std::vector<call_record>& log;
    };

    struct mock_bind_map_factory : i_factory<i_bind_map> {
        mutable mock_bind_map* last_created = nullptr;

        std::unique_ptr<i_bind_map> make() const override {
            auto m = std::make_unique<mock_bind_map>();
            last_created = m.get();
            return m;
        }
    };
}

TEST_CASE("overlay_bind_map") {
    std::vector<call_record> log;
    mock_bind_map_factory factory;

    bindings b;
    b.bind<i_factory<i_bind_map>>(factory);
    b.bind<std::vector<call_record>>(log);
    
    locator::register_bindings(&b);

    mock_bind_map remote;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr func{expr::functor{"f", {}}};
    expr func2{expr::functor{"g", {}}};

    overlay_bind_map obm(remote);
    auto* local = factory.last_created;

    SUBCASE("bind is forwarded to local only; remote receives no bind call") {
        obm.bind(0, &func);
        REQUIRE(log.size() == 1);
        REQUIRE(std::holds_alternative<bind_call>(log[0].call));
        auto& c = std::get<bind_call>(log[0].call);
        CHECK(log[0].source == local);
        CHECK(c.index       == 0);
        CHECK(c.e           == &func);
    }

    SUBCASE("whnf: local consulted first, resolves, remote not consulted") {
        local->whnf_responses[&var0] = &func;
        CHECK(obm.whnf(&var0) == &func);
        REQUIRE(log.size() == 1);
        REQUIRE(std::holds_alternative<whnf_call>(log[0].call));
        CHECK(log[0].source == local);
    }

    SUBCASE("whnf: local consulted first, then remote in order") {
        remote.whnf_responses[&var0] = &func;
        CHECK(obm.whnf(&var0) == &func);
        REQUIRE(log.size() == 2);
        REQUIRE(std::holds_alternative<whnf_call>(log[0].call));
        REQUIRE(std::holds_alternative<whnf_call>(log[1].call));
        CHECK(log[0].source                          == local);
        CHECK(std::get<whnf_call>(log[0].call).input == &var0);
        CHECK(log[1].source                          == &remote);
        CHECK(std::get<whnf_call>(log[1].call).input == &var0);
    }

    SUBCASE("whnf: local resolution shadows remote; remote never consulted") {
        local->whnf_responses[&var0]  = &func;
        remote.whnf_responses[&var0]  = &func2;
        CHECK(obm.whnf(&var0) == &func);
        REQUIRE(log.size() == 1);
        REQUIRE(std::holds_alternative<whnf_call>(log[0].call));
        CHECK(log[0].source == local);
    }

    SUBCASE("whnf: functor — local consulted first, then remote") {
        CHECK(obm.whnf(&func) == &func);
        REQUIRE(log.size() == 2);
        REQUIRE(std::holds_alternative<whnf_call>(log[0].call));
        REQUIRE(std::holds_alternative<whnf_call>(log[1].call));
        CHECK(log[0].source                          == local);
        CHECK(std::get<whnf_call>(log[0].call).input == &func);
        CHECK(log[1].source                          == &remote);
        CHECK(std::get<whnf_call>(log[1].call).input == &func);
    }

    SUBCASE("two overlays have independent locals; bind goes to own local only") {
        overlay_bind_map obm2(remote);
        auto* local2 = factory.last_created;

        obm.bind(0, &func);
        obm2.bind(0, &func2);

        REQUIRE(log.size() == 2);
        REQUIRE(std::holds_alternative<bind_call>(log[0].call));
        REQUIRE(std::holds_alternative<bind_call>(log[1].call));
        CHECK(log[0].source                        == local);
        CHECK(std::get<bind_call>(log[0].call).e   == &func);
        CHECK(log[1].source                        == local2);
        CHECK(std::get<bind_call>(log[1].call).e   == &func2);
    }

    locator::register_bindings(nullptr);
}
