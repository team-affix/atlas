#include <deque>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_exporter.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/ubs_class.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t, uint32_t));
};
struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor, (uint32_t, (const std::vector<const expr*>&)));
};
struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t));
};
struct MockFind {
    MOCK_METHOD(ubs_class, find, (uint32_t));
};

uint32_t offset_index(uint32_t frame_offset, uint32_t index) {
    return frame_offset + index;
}

ubs_class singleton(uint32_t var) {
    return ubs_class{.root = var, .size = 1, .rep = var, .value = std::nullopt};
}

using test_exporter_t = pud_exporter<
    NiceMock<MockGlobalize>, NiceMock<MockMakeFunctor>, NiceMock<MockMakeVar>, NiceMock<MockFind>>;

struct PudExporterTest : public ::testing::Test {
    std::deque<expr> owned;
    NiceMock<MockGlobalize> globalize;
    NiceMock<MockMakeFunctor> make_functor;
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockFind> find;
    test_exporter_t exporter{globalize, make_functor, make_var, find, 10};

    void SetUp() override {
        ON_CALL(globalize, globalize(_, _)).WillByDefault(Invoke(&offset_index));
        ON_CALL(find, find(_)).WillByDefault(Invoke(&singleton));
        ON_CALL(make_var, make_var(_)).WillByDefault(Invoke(this, &PudExporterTest::var));
    }

    const expr* var(uint32_t index) {
        owned.push_back(expr{expr::var{index}});
        return &owned.back();
    }

    const expr* functor(uint32_t id, std::vector<const expr*> args) {
        owned.push_back(expr{expr::functor{id, std::move(args)}});
        return &owned.back();
    }
};

}

TEST_F(PudExporterTest, AClassRepresentedBelowTheLvcIsTheCallersVariable) {
    const expr* caller_var = var(3);
    ON_CALL(find, find(14)).WillByDefault(Return(ubs_class{.root = 14, .size = 2, .rep = 3, .value = std::nullopt}));
    EXPECT_CALL(make_var, make_var(3u)).WillOnce(Return(caller_var));
    EXPECT_EQ(exporter.export_expr({var(4), 10}), caller_var);
    EXPECT_EQ(exporter.fresh_var_count(), 0u);
}

TEST_F(PudExporterTest, ACallerClassIsKeptEvenWhenBound) {
    const expr* caller_var = var(3);
    ON_CALL(find, find(3)).WillByDefault(Return(ubs_class{.root = 3, .size = 1, .rep = 3, .value = framed_expr{functor(1, {}), 0}}));
    EXPECT_CALL(make_var, make_var(3u)).WillOnce(Return(caller_var));
    EXPECT_CALL(make_functor, make_functor(_, _)).Times(0);
    EXPECT_EQ(exporter.export_expr({var(3), 0}), caller_var);
}

TEST_F(PudExporterTest, UnboundCalleeClassesAreNumberedDenselyFromTheLvcInFirstMetOrder) {
    EXPECT_CALL(make_var, make_var(10u)).Times(2);
    EXPECT_CALL(make_var, make_var(11u)).Times(1);
    exporter.export_expr({functor(1, {var(7), var(4), var(7)}), 10});
    EXPECT_EQ(exporter.fresh_var_count(), 2u);
}

TEST_F(PudExporterTest, AFreshVariableIsKeptForItsClassAcrossExports) {
    ON_CALL(find, find(16)).WillByDefault(Return(ubs_class{.root = 15, .size = 2, .rep = 15, .value = std::nullopt}));
    EXPECT_CALL(make_var, make_var(10u)).Times(2);
    exporter.export_expr({var(5), 10});
    exporter.export_expr({var(6), 10});
    EXPECT_EQ(exporter.fresh_var_count(), 1u);
}

TEST_F(PudExporterTest, ABoundCalleeClassIsReplacedByItsValueReadInItsOwnFrame) {
    const expr* fresh = var(10);
    const expr* rebuilt = functor(2, {fresh});
    ON_CALL(find, find(12)).WillByDefault(Return(ubs_class{.root = 12, .size = 1, .rep = 12, .value = framed_expr{functor(2, {var(5)}), 20}}));
    EXPECT_CALL(make_var, make_var(10u)).WillOnce(Return(fresh));
    EXPECT_CALL(make_functor, make_functor(2u, std::vector<const expr*>{fresh})).WillOnce(Return(rebuilt));
    EXPECT_EQ(exporter.export_expr({var(2), 10}), rebuilt);
}

TEST_F(PudExporterTest, ASharedSubtermIsRebuiltOnceAcrossExports) {
    const expr* shared = functor(2, {});
    const expr* rebuilt_shared = functor(2, {});
    EXPECT_CALL(make_functor, make_functor(2u, _)).WillOnce(Return(rebuilt_shared));
    EXPECT_EQ(exporter.export_expr({shared, 10}), rebuilt_shared);
    EXPECT_EQ(exporter.export_expr({shared, 10}), rebuilt_shared);
}
