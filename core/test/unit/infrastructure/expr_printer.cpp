#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sstream>
#include "../../../core/hpp/infrastructure/expr_printer.hpp"
#include "../../../core/hpp/interfaces/i_var_names.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockVarNames : public i_var_names {
    MOCK_METHOD(bool, is_named, (uint32_t), (const, override));
    MOCK_METHOD(const std::string&, name, (uint32_t), (const, override));
    MOCK_METHOD(void, set_name, (uint32_t, const std::string&), (override));
};

static std::string print_expr(i_var_names& names, const expr* e) {
    std::ostringstream os;
    expr_printer printer(os, names);
    printer.print(e);
    return os.str();
}

struct ExprPrinterTest : public ::testing::Test {
    MockVarNames names;
    std::string var_name_x{"X"};

    expr var0{expr::var{0}};
    expr atom_a{expr::functor{"a", {}}};
    expr atom_b{expr::functor{"b", {}}};
    expr nil{expr::functor{"nil", {}}};
};

TEST_F(ExprPrinterTest, PrintUnnamedVar) {
    EXPECT_CALL(names, is_named(0)).WillOnce(Return(false));

    EXPECT_EQ(print_expr(names, &var0), "?0");
}

TEST_F(ExprPrinterTest, PrintNamedVar) {
    EXPECT_CALL(names, is_named(0)).WillOnce(Return(true));
    EXPECT_CALL(names, name(0)).WillOnce(ReturnRef(var_name_x));

    EXPECT_EQ(print_expr(names, &var0), "X");
}

TEST_F(ExprPrinterTest, PrintAtom) {
    EXPECT_EQ(print_expr(names, &atom_a), "a");
}

TEST_F(ExprPrinterTest, PrintNil) {
    EXPECT_EQ(print_expr(names, &nil), "[]");
}

TEST_F(ExprPrinterTest, PrintSingletonList) {
    expr list{expr::functor{"cons", {&atom_a, &nil}}};
    EXPECT_EQ(print_expr(names, &list), "[a]");
}

TEST_F(ExprPrinterTest, PrintListWithTail) {
    expr list{expr::functor{"cons", {&atom_a, &atom_b}}};
    EXPECT_EQ(print_expr(names, &list), "[a|b]");
}

TEST_F(ExprPrinterTest, PrintMultiElementList) {
    expr tail{expr::functor{"cons", {&atom_b, &nil}}};
    expr list{expr::functor{"cons", {&atom_a, &tail}}};
    EXPECT_EQ(print_expr(names, &list), "[a, b]");
}

TEST_F(ExprPrinterTest, PrintGeneralFunctor) {
    expr f{expr::functor{"f", {&atom_a, &var0}}};

    EXPECT_CALL(names, is_named(0)).WillOnce(Return(true));
    EXPECT_CALL(names, name(0)).WillOnce(ReturnRef(var_name_x));

    EXPECT_EQ(print_expr(names, &f), "f(a, X)");
}
