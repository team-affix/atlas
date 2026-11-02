#include <cstdint>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_term_exporter.hpp"
#include "pud_identity_demoduler.hpp"
#include "value_objects/expr.hpp"
#include "pud_moduled_functor_helper.hpp"
#include "value_objects/ubs_class.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Pair;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

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

using test_exporter_t = pud_term_exporter<
    NiceMock<MockGlobalize>, NiceMock<MockMakeFunctor>, NiceMock<MockMakeVar>, NiceMock<MockFind>,
    pud_identity_functor_relinker>;

struct PudTermExporterTest : public ::testing::Test {
    std::deque<expr> owned;
    NiceMock<MockGlobalize> globalize;
    NiceMock<MockMakeFunctor> make_functor;
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockFind> find;
    pud_identity_functor_relinker functor_relinker;
    std::unordered_map<uint32_t, uint32_t> translations;
    test_exporter_t exporter{globalize, make_functor, make_var, find, functor_relinker, 10, translations};

    void SetUp() override {
        ON_CALL(globalize, globalize(_, _)).WillByDefault(Invoke(&offset_index));
        ON_CALL(find, find(_)).WillByDefault(Invoke(&singleton));
        ON_CALL(make_var, make_var(_)).WillByDefault(Invoke(this, &PudTermExporterTest::var));
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

TEST_F(PudTermExporterTest, AClassRepresentedBelowTheLvcIsTheCallersVariable) {
    const expr* caller_var = var(3);
    ON_CALL(find, find(14)).WillByDefault(Return(ubs_class{.root = 14, .size = 2, .rep = 3, .value = std::nullopt}));
    EXPECT_CALL(make_var, make_var(3u)).WillOnce(Return(caller_var));
    EXPECT_EQ(exporter.export_expr({var(4), 10, 0}, 0), caller_var);
    EXPECT_THAT(translations, IsEmpty());
}

TEST_F(PudTermExporterTest, ACallerClassIsKeptEvenWhenBound) {
    const expr* caller_var = var(3);
    ON_CALL(find, find(3)).WillByDefault(Return(ubs_class{.root = 3, .size = 1, .rep = 3, .value = pud_test_moduled_functor(functor(1, {}), 0)}));
    EXPECT_CALL(make_var, make_var(3u)).WillOnce(Return(caller_var));
    EXPECT_CALL(make_functor, make_functor(_, _)).Times(0);
    EXPECT_EQ(exporter.export_expr({var(3), 0, 0}, 0), caller_var);
}

TEST_F(PudTermExporterTest, UnboundCalleeClassesAreNumberedDenselyFromTheLvcInFirstMetOrder) {
    EXPECT_CALL(make_var, make_var(10u)).Times(2);
    EXPECT_CALL(make_var, make_var(11u)).Times(1);
    exporter.export_expr({functor(1, {var(7), var(4), var(7)}), 10, 0}, 0);
    EXPECT_THAT(translations, UnorderedElementsAre(Pair(17u, 10u), Pair(14u, 11u)));
}

TEST_F(PudTermExporterTest, AFreshVariableIsKeptForItsClassAcrossExports) {
    ON_CALL(find, find(16)).WillByDefault(Return(ubs_class{.root = 15, .size = 2, .rep = 15, .value = std::nullopt}));
    EXPECT_CALL(make_var, make_var(10u)).Times(2);
    exporter.export_expr({var(5), 10, 0}, 0);
    exporter.export_expr({var(6), 10, 0}, 0);
    EXPECT_THAT(translations, UnorderedElementsAre(Pair(15u, 10u)));
}

TEST_F(PudTermExporterTest, ATranslationAlreadyGivenIsReusedAndNumberingContinuesAfterIt) {
    translations.emplace(14, 10);
    EXPECT_CALL(make_var, make_var(10u));
    EXPECT_CALL(make_var, make_var(11u));
    exporter.export_expr({functor(1, {var(4), var(5)}), 10, 0}, 0);
    EXPECT_THAT(translations, UnorderedElementsAre(Pair(14u, 10u), Pair(15u, 11u)));
}

TEST_F(PudTermExporterTest, ABoundCalleeClassIsReplacedByItsValueReadInItsOwnFrame) {
    const expr* bound_shape = functor(2, {var(5)});
    const expr* fresh = var(10);
    const expr* rebuilt = functor(2, {fresh});
    ON_CALL(find, find(12)).WillByDefault(Return(ubs_class{
        .root = 12, .size = 1, .rep = 12, .value = pud_test_moduled_functor(bound_shape, 20)}));
    InSequence in_order;
    EXPECT_CALL(make_functor, make_functor(2u, _)).WillOnce(Return(bound_shape));
    EXPECT_CALL(make_var, make_var(10u)).WillOnce(Return(fresh));
    EXPECT_CALL(make_functor, make_functor(2u, std::vector<const expr*>{fresh})).WillOnce(Return(rebuilt));
    EXPECT_EQ(exporter.export_expr({var(2), 10, 0}, 0), rebuilt);
}

TEST_F(PudTermExporterTest, ASharedSubtermIsRebuiltOnceAcrossExports) {
    const expr* shared = functor(2, {});
    const expr* rebuilt_shared = functor(2, {});
    EXPECT_CALL(make_functor, make_functor(2u, _)).WillOnce(Return(rebuilt_shared));
    EXPECT_EQ(exporter.export_expr({shared, 10, 0}, 0), rebuilt_shared);
    EXPECT_EQ(exporter.export_expr({shared, 10, 0}, 0), rebuilt_shared);
}

struct MockRelinkFunctor {
    MOCK_METHOD(uint32_t, relink, (pud_module_id, uint32_t, pud_module_id));
};

using translating_exporter_t = pud_term_exporter<
    NiceMock<MockGlobalize>, NiceMock<MockMakeFunctor>, NiceMock<MockMakeVar>, NiceMock<MockFind>, MockRelinkFunctor>;

struct PudTermExporterTranslationTest : public ::testing::Test {
    std::deque<expr> owned;
    NiceMock<MockGlobalize> globalize;
    NiceMock<MockMakeFunctor> make_functor;
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockFind> find;
    MockRelinkFunctor relinker;
    std::unordered_map<uint32_t, uint32_t> translations;
    translating_exporter_t exporter{globalize, make_functor, make_var, find, relinker, 10, translations};

    const expr* var(uint32_t index) {
        owned.push_back(expr{expr::var{index}});
        return &owned.back();
    }

    const expr* functor(uint32_t id, std::vector<const expr*> args) {
        owned.push_back(expr{expr::functor{id, std::move(args)}});
        return &owned.back();
    }

    void SetUp() override {
        ON_CALL(globalize, globalize(_, _)).WillByDefault(Invoke(&offset_index));
        ON_CALL(find, find(_)).WillByDefault(Invoke(&singleton));
        ON_CALL(make_var, make_var(_)).WillByDefault(Invoke(this, &PudTermExporterTranslationTest::var));
    }
};

TEST_F(PudTermExporterTranslationTest, EachFunctorNodeIsTranslatedAndAVariableIsNot) {
    const expr* variable = var(0);
    const expr* inner = functor(6, {variable});
    const expr* right = functor(7, {});
    const expr* term = functor(5, {inner, right});
    const expr* translated_variable = var(0);
    const expr* translated_inner = functor(16, {translated_variable});
    const expr* translated_right = functor(17, {});
    const expr* translated = functor(15, {translated_inner, translated_right});
    InSequence order;
    EXPECT_CALL(make_var, make_var(0u)).WillOnce(Return(translated_variable));
    EXPECT_CALL(relinker, relink(3u, 6u, 8u)).WillOnce(Return(16u));
    EXPECT_CALL(make_functor, make_functor(16u, std::vector<const expr*>{translated_variable})).WillOnce(Return(translated_inner));
    EXPECT_CALL(relinker, relink(3u, 7u, 8u)).WillOnce(Return(17u));
    EXPECT_CALL(make_functor, make_functor(17u, std::vector<const expr*>{})).WillOnce(Return(translated_right));
    EXPECT_CALL(relinker, relink(3u, 5u, 8u)).WillOnce(Return(15u));
    EXPECT_CALL(make_functor, make_functor(15u, std::vector<const expr*>{translated_inner, translated_right}))
        .WillOnce(Return(translated));
    EXPECT_EQ(exporter.export_expr(moduled_expr{term, 0, 3}, 8), translated);
}

TEST_F(PudTermExporterTranslationTest, ASecondExportIsASeparateWalk) {
    const expr* first = functor(5, {});
    const expr* second = functor(5, {});
    const expr* translated = functor(15, {});
    EXPECT_CALL(relinker, relink(3u, 5u, 8u)).Times(2).WillRepeatedly(Return(15u));
    EXPECT_CALL(make_functor, make_functor(15u, std::vector<const expr*>{})).Times(2).WillRepeatedly(Return(translated));
    EXPECT_EQ(exporter.export_expr(moduled_expr{first, 0, 3}, 8), translated);
    EXPECT_EQ(exporter.export_expr(moduled_expr{second, 0, 3}, 8), translated);
}

TEST_F(PudTermExporterTranslationTest, AVariableIsNotRelinked) {
    EXPECT_CALL(relinker, relink(_, _, _)).Times(0);
    EXPECT_CALL(make_var, make_var(4u)).WillOnce(Return(var(4)));
    ON_CALL(find, find(4)).WillByDefault(Return(ubs_class{.root = 4, .size = 1, .rep = 4, .value = std::nullopt}));
    exporter.export_expr(moduled_expr{var(4), 0, 3}, 8);
}
