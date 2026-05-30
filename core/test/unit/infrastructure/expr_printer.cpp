// expr_printer renders expr trees to a stream, consulting i_var_names for variables.
// Unit tests mock var_names and assert string output for atoms, lists, and functors.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sstream>
#include "locator_fixture.hpp"
#include "infrastructure/expr_printer.hpp"
#include "interfaces/i_var_names.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockVarNames : public i_var_names {
    MOCK_METHOD(bool, is_named, (uint32_t), (const, override));
    MOCK_METHOD(const std::string&, name, (uint32_t), (const, override));
    MOCK_METHOD(void, set_name, (uint32_t, const std::string&), (override));
};

struct ExprPrinterTest : public ::testing::Test {
    locator loc;
    MockVarNames names;
    std::ostringstream os;
    expr_printer printer;

    ExprPrinterTest() : printer(init_printer()) {}

    expr_printer init_printer() {
        loc.bind_as<i_var_names>(names);
        return expr_printer{os, loc};
    }
    std::string var_name_x{"X"};

    expr var0{expr::var{0}};
    expr atom_a{expr::functor{"a", {}}};
    expr atom_b{expr::functor{"b", {}}};
    expr nil{expr::functor{"nil", {}}};

    std::string print(const expr* e) {
        os.str("");
        os.clear();
        printer.print(e);
        return os.str();
    }
};

TEST_F(ExprPrinterTest, PrintUnnamedVar) {
    EXPECT_CALL(names, is_named(0)).WillOnce(Return(false));

    EXPECT_EQ(print(&var0), "?0");
}

TEST_F(ExprPrinterTest, PrintNamedVar) {
    EXPECT_CALL(names, is_named(0)).WillOnce(Return(true));
    EXPECT_CALL(names, name(0)).WillOnce(ReturnRef(var_name_x));

    EXPECT_EQ(print(&var0), "X");
}

TEST_F(ExprPrinterTest, PrintAtom) {
    EXPECT_EQ(print(&atom_a), "a");
}

TEST_F(ExprPrinterTest, PrintNil) {
    EXPECT_EQ(print(&nil), "[]");
}

TEST_F(ExprPrinterTest, PrintSingletonList) {
    expr list{expr::functor{"cons", {&atom_a, &nil}}};
    EXPECT_EQ(print(&list), "[a]");
}

TEST_F(ExprPrinterTest, PrintListWithTail) {
    expr list{expr::functor{"cons", {&atom_a, &atom_b}}};
    EXPECT_EQ(print(&list), "[a|b]");
}

TEST_F(ExprPrinterTest, PrintMultiElementList) {
    expr tail{expr::functor{"cons", {&atom_b, &nil}}};
    expr list{expr::functor{"cons", {&atom_a, &tail}}};
    EXPECT_EQ(print(&list), "[a, b]");
}

TEST_F(ExprPrinterTest, PrintGeneralFunctor) {
    expr f{expr::functor{"f", {&atom_a, &var0}}};

    EXPECT_CALL(names, is_named(0)).WillOnce(Return(true));
    EXPECT_CALL(names, name(0)).WillOnce(ReturnRef(var_name_x));

    EXPECT_EQ(print(&f), "f(a, X)");
}

TEST_F(ExprPrinterTest, PrintTernaryFunctor) {
    expr f3{expr::functor{"h", {&atom_a, &atom_b, &var0}}};

    EXPECT_CALL(names, is_named(0)).WillOnce(Return(true));
    EXPECT_CALL(names, name(0)).WillOnce(ReturnRef(var_name_x));

    EXPECT_EQ(print(&f3), "h(a, b, X)");
}

TEST_F(ExprPrinterTest, PrintFourElementList) {
    expr atom_c{expr::functor{"c", {}}};
    expr atom_d{expr::functor{"d", {}}};
    expr tail{expr::functor{"cons", {&atom_d, &nil}}};
    expr t2{expr::functor{"cons", {&atom_c, &tail}}};
    expr t1{expr::functor{"cons", {&atom_b, &t2}}};
    expr list{expr::functor{"cons", {&atom_a, &t1}}};
    EXPECT_EQ(print(&list), "[a, b, c, d]");
}

TEST_F(ExprPrinterTest, PrintNestedFunctorArgs) {
    expr inner{expr::functor{"g", {&atom_a, &atom_b}}};
    expr outer{expr::functor{"f", {&inner, &var0}}};

    EXPECT_CALL(names, is_named(0)).WillOnce(Return(true));
    EXPECT_CALL(names, name(0)).WillOnce(ReturnRef(var_name_x));

    EXPECT_EQ(print(&outer), "f(g(a, b), X)");
}
