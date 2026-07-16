// unifier_factory: constructs a unifier wired to the supplied bind map. Unification
// must delegate WHNF lookups to that bind map collaborator.

#include <unordered_set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/globalizer.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "functor_fixture.hpp"

using ::testing::Return;
using ::testing::_;

namespace {

struct MockBindMap {
    MOCK_METHOD(void, bind, (uint32_t, framed_expr));
    MOCK_METHOD(framed_expr, whnf, (framed_expr));
};

using TestUnifierFactory = unifier_factory<globalizer, MockBindMap>;
using TestUnifier = unifier<globalizer, MockBindMap>;

bool run_unify(TestUnifier& u, const expr* lhs, const expr* rhs,
               std::unordered_set<uint32_t>& vars_touched) {
    auto task = u.unify({lhs, 0}, {rhs, 0});
    while (!task.done()) {
        task.resume();
        if (task.has_yield())
            vars_touched.insert(task.consume_yield());
    }
    return task.result();
}

} // namespace

struct UnifierFactoryTest : public ::testing::Test {
    test_functors functors;
    globalizer g;
    TestUnifierFactory factory{g};
    MockBindMap bind_map;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr func_f{expr::functor{functors.id("f"), {}}};
    expr func_g{expr::functor{functors.id("g"), {}}};
};

TEST_F(UnifierFactoryTest, MakeProducesUnifierThatUnifiesViaBindMap) {
    EXPECT_CALL(bind_map, whnf).WillRepeatedly([](framed_expr fe) { return fe; });
    EXPECT_CALL(bind_map, bind(1u, framed_expr{&var0, 0})).Times(1);
    TestUnifier u = factory.make(&bind_map);
    std::unordered_set<uint32_t> vars_touched;
    EXPECT_TRUE(run_unify(u, &var0, &var1, vars_touched));
}

TEST_F(UnifierFactoryTest, MakeProducesUnifierThatReportsUnifyFailure) {
    EXPECT_CALL(bind_map, whnf).WillRepeatedly([](framed_expr fe) { return fe; });
    EXPECT_CALL(bind_map, bind(_, _)).Times(0);
    TestUnifier u = factory.make(&bind_map);
    std::unordered_set<uint32_t> vars_touched;
    EXPECT_FALSE(run_unify(u, &func_f, &func_g, vars_touched));
}
